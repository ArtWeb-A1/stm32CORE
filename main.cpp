#include <stdint.h>
#include <stddef.h>
#include "stm32h5xx_hal.h"

// ==================== [Variables] ==================== //

#define MAX_TIDs_PER_PROCESS 4U
#define MAX_MPU_REGIONS      12U
#define MAX_PROCESS_IN_LIST  16U
#define MAX_THREADS_IN_LIST  64U 

#define READY               0x00

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
	ThreadControlBlock* TIDs[MAX_TIDs_PER_PROCESS];
	uint16_t TLast;
}; 

	// ==================== [Constants] ==================== //
	

#define MPURegionConfigSize      (sizeof(MPURegionConfig) * 8U)       // Размер структуры определения региона в битах.
#define RegisterContextSize      (sizeof(RegisterContext) * 8U)       // Размер 
#define ProcessControlBlockSize  (sizeof(ProcessControlBlock) * 8U)
#define ThreadControlBlockSize   (sizeof(ThreadControlBlock) * 8U)
#define TSP_OFFSET \
    offsetof(ThreadControlBlock, TSP)

#define TR_OFFSET \
    offsetof(ThreadControlBlock, TR)

#define EXC_RETURN_OFFSET \
    (offsetof(ThreadControlBlock, TR) + \
     offsetof(RegisterContext, EXC_RETURN))
	 
static_assert(offsetof(ThreadControlBlock, TSP) % 4 == 0,
              "TSP must be word-aligned");

static_assert(offsetof(ThreadControlBlock, TR) % 4 == 0,
              "TR must be word-aligned");

MPURegionConfig     MPU_regions[MAX_MPU_REGIONS]     {};        // Массив с текущими регионами (включая выключенных).
ProcessControlBlock ProcessList[MAX_PROCESS_IN_LIST] {}; // Массив со всеми текущими процессами.
ThreadControlBlock  ThreadList[MAX_THREADS_IN_LIST]  {};    // Массив со всеми текущими потоками.

uint16_t ProcessCount   = 0;
uint16_t ProcessLast    = 65535;
uint16_t ThreadLast     = 65535;

ThreadControlBlock* last_thread    = nullptr;
ThreadControlBlock* current_thread = nullptr;

// ==================== [Call func prototypes] ==================== //

// Базовое значение кванта для тестирования 1мс. В будующем будет меняться по мере развития структуры и тестирования.

// В текущий момент реализована системма переключения по потоком, что лучше распределяет ресурсы по потокам.
// Программа А имеет 3 потока.
// Программа Б имеет 1 поток.
// А1 -> Б1 -> А2 -> Б1 -> А3

extern "C" __attribute__((naked))
void PendSV_Handler(void)
{
    asm volatile(
        // =========================================
        // 1. Сохранить контекст last_thread
        // =========================================

        "ldr   r0, =last_thread       \n"
        "ldr   r0, [r0]               \n"
        "cbz   r0, 1f                 \n"

        // Сохранить PSP
        "mrs   r1, psp                \n"
        "str   r1, [r0, #16]          \n" // TSP

        // Адрес last_thread->TR
        "add   r2, r0, #28            \n"

        // Сохранить R4-R11
        "stmia r2, {r4-r11}           \n"

        // Сохранить EXC_RETURN
        "str   lr, [r2, #32]          \n"

        // =========================================
        // 2. Получить current_thread
        // =========================================

        "1:                           \n"
        "ldr   r0, =current_thread    \n"
        "ldr   r0, [r0]               \n"
        "cbz   r0, 2f                 \n"

        // =========================================
        // 3. Восстановить стек нового потока
        // =========================================

        "ldr   r1, [r0, #16]          \n" // TSP
        "msr   psp, r1                \n"

        // Адрес current_thread->TR
        "add   r2, r0, #28            \n"

        // Восстановить R4-R11
        "ldmia r2, {r4-r11}           \n"

        // Восстановить EXC_RETURN
        "ldr   lr, [r2, #32]          \n"

        // =========================================
        // 4. Возврат в новый поток
        // =========================================

        "bx    lr                     \n"

        // Нет текущего потока
        "2:                           \n"
        "bx    lr                     \n"
    );
}


/*
 * TODO: Подготовка и развитие RTOS.
 *
 * 1.1 При первом запуске потока нужно подготовить начальный контекст,
 *     чтобы процессор мог запустить поток через механизм возврата
 *     из исключения. Для этого необходимо заполнить стек начальными
 *     значениями регистров, PC, xPSR и EXC_RETURN.
 *
 * 1.2 Добавить инициализацию системных структур RTOS. Предусмотреть
 *     создание Idle-процесса и Idle-потока, резервирование PID для
 *     системных задач, подготовку начальных стеков, настройку
 *     приоритетов потоков и конфигурацию необходимых исключений.
 *     Создание процессов и потоков вынести в отдельные функции.
 *
 * 1.3 Реализовать сохранение и восстановление полного контекста потока.
 *     Определить, какие регистры и системные значения нужно сохранять
 *     при переключении, включая R4-R11, PSP и EXC_RETURN. Если потоки
 *     используют FPU, предусмотреть сохранение и восстановление
 *     состояния регистров FPU и связанных с ним параметров.
 *
 * 1.4 Добавить контроль стеков потоков. Для каждого стека определить
 *     начальную и конечную границы, проверять корректность указателя
 *     стека перед использованием и обнаруживать выход за допустимые
 *     пределы. Предусмотреть обработку переполнения стека, чтобы
 *     повреждение памяти не оставалось незамеченным.
 *
 * 1.5 Реализовать обработку ошибок планировщика и переключения контекста.
 *     Предусмотреть действия на случай отсутствия готовых потоков,
 *     некорректного TCB, повреждённого контекста или невозможности
 *     восстановить состояние потока. Определить, когда следует
 *     запускать Idle-поток, а когда останавливать систему или вызывать
 *     обработчик критической ошибки.
 *
 * 2. Реализовать управление состояниями процессов и потоков. Определить
 *    допустимые состояния, правила переходов между ними и условия,
 *    при которых поток может быть выбран планировщиком. Обеспечить
 *    корректное обновление состояния при запуске, блокировке,
 *    завершении и возобновлении потока.
 *
 * 3. Реализовать управление приоритетами потоков и выбор следующего
 *    потока на выполнение. Планировщик должен учитывать состояние
 *    потока и его приоритет, выбирать подходящий готовый поток
 *    и обеспечивать выполнение Idle-потока, когда других готовых
 *    потоков нет.
 *
 * 4. Реализовать механизм переключения контекста через PendSV.
 *    Планировщик должен определять следующий поток, а PendSV_Handler —
 *    сохранять контекст текущего потока и восстанавливать контекст
 *    выбранного потока. Предусмотреть отдельный путь для первого
 *    запуска, когда предыдущего потока ещё нет.
 *
 * 5. Реализовать учёт времени выполнения процессов и потоков.
 *    Определить, как измерять использованное процессорное время,
 *    обновлять счётчики TCpuT и PCpuT и связывать измерения
 *    с конкретным потоком и его процессом.
 *
 * 6. Реализовать проверки целостности списков процессов и потоков.
 *    Проверять корректность PID и TID, соответствие потоков процессам,
 *    допустимость индексов и указателей, а также согласованность
 *    счётчиков и занятых элементов массивов. Предусмотреть обработку
 *    ситуации, когда системные таблицы заполнены или содержат ошибки.
 *
 * 7. Добавить поддержку TrustZone для переключения между Secure
 *    и Non-secure окружениями. Предусмотреть хранение и восстановление
 *    состояния безопасности потока, использование соответствующих
 *    стеков PSP_S и PSP_NS и корректное переключение security state.
 *    Отдельно учесть различия контекстов Secure и Non-secure при
 *    сохранении и восстановлении потока.
 *
 * Планировщик отвечает за выбор следующего готового потока.
 * PendSV_Handler отвечает за сохранение и восстановление контекста.
 * Инициализация системы должна подготовить структуры и начальные
 * контексты до первого запуска планировщика.
 */

ThreadControlBlock *scheduler(void) 
{
	bool ThreadIsReady = false;

	if (ProcessCount == 0)
	{
		// Вызвать PID 0 idle
		return NULL;
	}
	
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
				
				if (ProcessList[pNum].TIDs[tNum] != NULL)               // Проверка существования потока.
				{
					if (ProcessList[pNum].TIDs[tNum]->TSTATE == READY)  // Проверка потока на готовность.
					{
						if (ProcessLast != 65535 && ThreadLast != 65535)
						{
							last_thread = ProcessList[ProcessLast].TIDs[ThreadLast];
						}
						
						current_thread = ProcessList[pNum].TIDs[tNum];
						if (last_thread != current_thread)
						{
							SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
						}
						
						// Логика переключения процесса
						ProcessLast             = pNum;             // Замена значения последнего процесса. 
						ThreadLast              = tNum; 
						ProcessList[pNum].TLast = tNum;             // Замена значения последнего потока.
						ThreadIsReady           = true;
						
						
						
						return ProcessList[pNum].TIDs[tNum];        // Завершаем цикл.
					}
				}
			}
		}
	}
	if (ThreadIsReady == false)                                    // Idle состояние.
	{
		// Вызывать PID 0 Idle.
		return NULL;
	}
	return NULL;
}

// ==================== [Main function] ==================== //

int main(void) 
{
    HAL_Init();


	// Скрипты чтобы узнать смещение блоков.
	// volatile uint32_t tsp_offset =
		// offsetof(ThreadControlBlock, TSP);

	// volatile uint32_t tr_offset =
		// offsetof(ThreadControlBlock, TR);

	// volatile uint32_t exc_return_offset =
		// offsetof(ThreadControlBlock, TR) +
		// offsetof(RegisterContext, EXC_RETURN);
	// static_assert(offsetof(ThreadControlBlock, TSP) == 17,
              // "TSP offset is not 16");

	// static_assert(offsetof(ThreadControlBlock, TR) == 28,
				  // "TR offset is not 28");
	
    while (1) {
    }
}

// ==================== [Another functions] ==================== //