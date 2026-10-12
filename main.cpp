#include <stdint.h>
#include "stm32h5xx_hal.h"

// ==================== [Variables] ==================== //

#define MAX_TIDs_PER_PROCESS 4U
#define MAX_MPU_REGIONS      12U
#define MAX_PROCESS_IN_LIST  16U
#define MAX_THREADS_IN_LIST  64U 

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
	RegisterContext TR;// Thread Registers       - Сохранение состояния регистров процессора для данного процесса.
	int16_t TS;     // Thread Signal            - Передача сигнала в процесс.
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
	uint32_t POF;    // Process Open Files      - Все открытые файл процессом.
	int16_t PS;     // Process Signal          - Передача сигнала в процесс.
	ThreadControlBlock TIDs[MAX_TIDs_PER_PROCESS];
	uint16_t TLast;
}; 

	// ==================== [Constants] ==================== //
	

#define MPURegionConfigSize      (sizeof(MPURegionConfig) * 8U)       // Размер структуры определения региона в битах.
#define RegisterContextSize      (sizeof(RegisterContext) * 8U)       // Размер 
#define ProcessControlBlockSize  (sizeof(ProcessControlBlock) * 8U)
#define ThreadControlBlockSize   (sizeof(ThreadControlBlock) * 8U)

MPURegionConfig     MPU_regions[MAX_MPU_REGIONS]     {};        // Массив с текущими регионами (включая выключенных).
ProcessControlBlock ProcessList[MAX_PROCESS_IN_LIST] {}; // Массив со всеми текущими процессами.
ThreadControlBlock  ThreadList[MAX_THREADS_IN_LIST]  {};    // Массив со всеми текущими потоками.

uint16_t ProcessCount   = 0;
uint16_t ProcessLast    = 0;

// ==================== [Call func prototypes] ==================== //

// Базовое значение кванта для тестирования 1мс. В будующем будет меняться по мере развития структуры и тестирования.

// В текущий момент реализована системма переключения по потоком, что лучше распределяет ресурсы по потокам.
// Программа А имеет 3 потока.
// Программа Б имеет 1 поток.
// А1 -> Б1 -> А2 -> Б1 -> А3

ThreadControlBlock *scheduler(void) 
{
	bool ThreadIsReady = false;
	for (uint16_t pCount=1; pCount<=ProcessCount; pCount++)         // Перебор всех процессов.
	{
		uint16_t pNum = (ProcessLast + pCount) % ProcessCount;		// Подсчет следующего процесса.
		// ProcessLast = pNum;
		if (ProcessList[pNum].PSTATE == READY)                      // Проверка процесса на готовность.
		{
			// Анализ статуса потока и запуск его выполнения.
			for (uint16_t tCount=1; tCount<=MAX_TIDs_PER_PROCESS; tCount++) // Перебор всех потоков процесса.
			{
				uint16_t tNum = (ProcessList[pNum].TLast + tCount) % MAX_TIDs_PER_PROCESS; // Подсчет следующего потока.
				
				if (ProcessList[pNum].TIDs[tNum] != NULL)           // Проверка существования потока.
				{
					if (ProcessList[pNum].TIDs[tNum]->TSTATE == READY)  // Проверка потока на готовность.
					{
						// Логика переключения процесса
						ProcessLast = pNum;                         // Замена значения последнего процесса. 
						ProcessList[pNum].TLast = tNum;             // Замена значения последнего потока.


						
						return ProcessList[pNum].TIDs[tNum];        // Завершаем цикл.
					}
				}
			}
		}
	}
	if (ThreadIsReady == false).                                    // Idle состояние.
	{
		// Вызывать PID 0 Idle.
	}
}

// ==================== [Main function] ==================== //

int main(void) 
{
    HAL_Init();


    while (1) {
    }
}

// ==================== [Another functions] ==================== //
