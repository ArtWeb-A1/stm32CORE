#include <stdint.h>
#include "stm32h5xx_hal.h"

// ==================== [Variables] ==================== //

	// ==================== [Structures] ==================== //


struct MPURegionConfig     // Структура определения региона.
{
	uint16_t PID;    // Process ID          - Процесс или родственный процесс владеющий регионом.
	uint16_t RN;     // Region Num          - Номер региона.
	uint32_t RBA;    // Region Base Address - Начальный адрес региона.
	uint32_t RSz;    // Region Size         - Размер региона.
	uint8_t RS;      // Region Shareability - "определяет; может ли память использоваться совместно несколькими агентами системы."
	uint8_t RA;      // Region Access       - Права чтения/записи/исполнения.
	uint8_t RMAIRI;  // Region MAIR Index   - "индекс MAIR".
	uint8_t RE;      // Region Enabled      - Включен ли регион.
	uint8_t RAttr;   // Region Attribute    - Атрибут региона.
};

struct RegisterContext     // Структура сохранения контекста регистров процессора.
{
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	uint32_t EXC_RETURN;
};

struct ProcessControlBlock // Структура определения процесса.
{
	uint16_t PID;    // Process Id       	    - Номер процесса.
	uint16_t PPID;   // Parent Process Id	    - Номер родительского процесса.
	uint8_t PSTATE;  // Process State     	    - Состояние процесса.
	uint8_t PP;      // Process Priority  	    - Приоретет процесса.
	uint64_t PCpuT;  // Process Cpu Time        - Рабочее время процесса на CPU. (Сохраняет время в микросекундах).
	uintptr_t  PSP;  // Process Stack Pointer   - Адрес стека.
	uintptr_t  PSL;  // Process Stack Last      - Последний адрес стэка.
	uint32_t PPC;    // Process Program Counter - Значение следующий инструкции процесса.
	uint32_t PTZ;    // Process Trust Zone      - Состояние trust зоны Secure/Non-secure.
//	RegisterContext PR; // Process Registers    - Сохранение состояния регистров процессора для данного процесса (нужно сохранять только у потоков так как процесс это маска для нескольких потоков).
	uint32_t POF;    // Process Open Files      - Все открытые файл процессом.
	int16_t PS;     // Process Signal          - Передача сигнала в процесс.
}; 

struct ThreadControlBlock  // Структура определения потока процесса.
{
	uint16_t PID;    // Process Id       	    - Номер процесса.
	uint16_t TID;    // Thread Id               - Номер потока процесса.
	uint16_t PTID;   // Parent Thread Id  	    - Номер родительского потока процесса.
	uint8_t TSTATE;  // Thread State      	    - Состояние потока процесса.
	uint8_t TP;      // Thread Priority  	    - Приоретет потока процесса.
	uint64_t TCpuT;  // Thread Cpu Time         - Рабочее время потока процесса на CPU. (Сохраняет время в микросекундах).
	uintptr_t  TSP;  // Process Stack Pointer   - Адрес стека у потока.
	uintptr_t  TSL;  // Thread Stack Last       - Последний адрес стэка у потока.
	uint32_t TPC;    // Thread Program Counter   - Значение следующий инструкции процесса.
	RegisterContext TR;// Thread Registers       - Сохранение состояния регистров процессора для данного процесса
//	uint32_t TOF;    // Thread Open Files        - Все открытые файл процессом. (все потоки будут иметь доступ к файлам процесса).
	int16_t TS;     // Thread Signal            - Передача сигнала в процесс.
};

// struct PCB_makeProcess							// Первоначальная версия таблици процесса.
// {
	// uint32_t PID;    // Process id       	    - Номер процесса.
	// uint32_t PPID;   // Parent Process id	    - Номер родительского процесса.
	// uint32_t TID;    // Thread id               - Номер потока процесса.
	// uint32_t PTID;   // Parent thread id  	    - Номер родительского потока процесса.
	// uint8_t PSTATE;  // Process state     	    - Состояние процесса.
	// uint8_t TSTATE;  // Thread state      	    - Состояние потока процесса.
	// uint8_t PP;      // Process priority  	    - Приоретет процесса.
	// uint8_t TP;      // Thread priority  	    - Приоретет потока процесса.
	// uint32_t PCpuT;  // Process Cpu time        - Рабочее время процесса на CPU.
	// uint32_t TCpuT;  // Thread Cpu time         - Рабочее время потока процесса на CPU.
	// uint32_t PSP;    // Process stack pointer   - Указатель адреса стека
	// uint32_t PPC;    // Process program counter - Указатель слудющей инструкции процесса.
	// PRS_saveRegisters PR;     // Process Registers       - Сохранение состояния регистров процессора для данного процесса
	// uint32_t POF;    // Process open files      - Все открытые файл процессом.
	// uint32_t PS;      // Process signal          - Передача сигнала в процесс.
// };

	// ==================== [Constants] ==================== //
	

#define MPURegionConfigSize      (sizeof(MPURegionConfig) * 8U)       // Размер структуры определения региона в битах.
#define RegisterContextSize      (sizeof(RegisterContext) * 8U)       // Размер 
#define ProcessControlBlockSize  (sizeof(ProcessControlBlock) * 8U)
#define ThreadControlBlockSize   (sizeof(ThreadControlBlock) * 8U)

MPURegionConfig MPU_regions[MPURegionConfigSize*12U] {};        // Массив с текущими регионами (включая выключенных).
ProcessControlBlock ProcessList[ProcessControlBlockSize*16U] {}; // Массив со всеми текущими процессами.
ThreadControlBlock ThreadList[ThreadControlBlockSize*64U] {};    // Массив со всеми текущими потоками.

uint16_t ProcessCount = 0;

// ==================== [Call func prototypes] ==================== //

// Базовое значение кванта для тестирования 1мс. В будующем будет меняться по мере развития структуры и тестирования.

ThreadControlBlock *scheduler(void) 
{
	bool ThreadIsReady = false;
	if (ProcessCount == 0)
	{
		// Вызывать PID 0 Idle.
	} else
	{
		for (uint16_t i=0; i<ProcessCount; i++)         // Перебор всех процессов.
		{
			if (ProcessList[i].PSTATE == PROCESS_READY) // PROCESS_READY - макрос значение для ready процесса.
			{
				// ProcessIsReady = true;
				// Процесс готов к выполнению. Нужно переходить к проверке потока.
			}
		}
		if (ThreadIsReady == false)
		{
			// Вызывать PID 0 Idle.
		}
	}
}

// void Secure_MPU_Init(void);


// ==================== [Main function] ==================== //

int main(void) 
{
    HAL_Init();


    while (1) {
    }
}

// ==================== [Another functions] ==================== //



// void Secure_MPU_Init(void)
// {
    // // Отключаем MPU, чтобы безопасно изменить его настройки.
    // // Это не отключает TrustZone и не меняет атрибуцию Secure/Non-secure.
    // ARM_MPU_Disable();

    // // MAIR0 — регистр атрибутов памяти.
    // // 0x44 задаёт Attr0 = 0x44.
    // // 0x44 означает Normal memory, non-cacheable.
    // // Регион с AttrIndx = 0 будет использовать этот атрибут.
    // MPU->MAIR0 = 0x44U;

    // // RNR (Region Number Register) выбирает регион MPU,
    // // настройки которого мы будем изменять.
    // // Значение 0 означает регион №0.
    // MPU->RNR = 0U;

    // // RBAR (Region Base Address Register) задаёт:
    // // базовый адрес региона, shareability, права доступа и XN.

    // MPU->RBAR =
        // (0x30000000U & MPU_RBAR_BASE_Msk) |
        // // Базовый адрес региона: 0x30000000.
        // // Маска оставляет только биты адреса, разрешённые RBAR.

        // (0U << MPU_RBAR_SH_Pos) |
        // // SH = 0: память не помечается как shareable.
        // // Это подходящий вариант для обычной локальной SRAM,
        // // если архитектура системы не требует другого.

        // (1U << MPU_RBAR_AP_Pos) |
        // // AP = 1: доступ на чтение и запись только
        // // из привилегированного режима.
        // // Непривилегированный код не получит доступ.

        // MPU_RBAR_XN_Msk;
        // // XN = 1: Execute Never.
        // // Запрещает выполнение инструкций из этого региона.

    // // RLAR (Region Limit Address Register) задаёт:
    // // верхнюю границу региона, индекс атрибута и включение региона.

    // MPU->RLAR =
        // (0x30007FFFU & MPU_RLAR_LIMIT_Msk) |
        // // Последний адрес региона — 0x30007FFF.
        // // Границы RLAR задаются с точностью 32 байта,
        // // младшие биты адреса не хранятся.
        // // В данном примере диапазон равен 32 КБ.

        // (0U << MPU_RLAR_AttrIndx_Pos) |
        // // AttrIndx = 0: использовать Attr0 из MAIR0.
        // // Поэтому память получит атрибут 0x44.

        // MPU_RLAR_EN_Msk;
        // // EN = 1: включить регион №0.

    // // DSB ждёт завершения предыдущих операций с памятью
    // // и завершения предыдущих обращений к периферии.
    // __DSB();

    // // ISB сбрасывает конвейер инструкций, чтобы последующие
    // // инструкции выполнялись с учётом новых настроек MPU.
    // __ISB();

    // // Включаем MPU.
    // // Аргумент 0 означает: PRIVDEFENA = 0.
    // // Не использовать стандартную карту памяти как fallback
    // // для привилегированных обращений, не покрытых регионом.
    // ARM_MPU_Enable(0U);

    // // Синхронизация после включения MPU.
    // __DSB();
    // __ISB();
// }
