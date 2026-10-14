#include "sched.h"

#define READY               0x00		// В будующем вынести в отдельный файл с глобальными перемеными. 
#define RUNNING             0x01

uint16_t ProcessCount   = 0;
uint16_t PIDCount       = 1;
uint8_t PIDFree[65536 / 8] = {0};		// 0 - PID свободен. 1 - PID занят.

uint16_t ProcessLast    = 65535;
uint16_t ThreadLast     = 65535;

volatile uint8_t switch_pending = 0;
volatile uint8_t quantum_left   = 0;

MPURegionConfig     MPU_regions[MAX_MPU_REGIONS]     {};        // Массив с текущими регионами (включая выключенных).
ProcessControlBlock ProcessList[MAX_PROCESS_IN_LIST] {};        // Массив со всеми текущими процессами.
ThreadControlBlock  ThreadList[MAX_THREADS_IN_LIST]  {};        // Массив со всеми текущими потоками.

ThreadControlBlock* last_thread    = nullptr;
ThreadControlBlock* current_thread = nullptr;

static_assert(offsetof(ThreadControlBlock, TSP) % 4 == 0,
              "TSP must be word-aligned");

static_assert(offsetof(ThreadControlBlock, TR) % 4 == 0,
              "TR must be word-aligned");
			  
// Базовое значение кванта для тестирования 1мс. В будующем будет меняться по мере развития структуры и тестирования.

// В текущий момент реализована системма переключения по потоком, что лучше распределяет ресурсы по потокам.
// Программа А имеет 3 потока.
// Программа Б имеет 1 поток.
// А1 -> Б1 -> А2 -> Б1 -> А3

extern "C" __attribute__((naked))
void PendSV_Handler(void)
{
    asm volatile(
        // 1. Сохранить контекст last_thread
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

        // 2. Получить current_thread
        "1:                           \n"
        "ldr   r0, =current_thread    \n"
        "ldr   r0, [r0]               \n"
        "cbz   r0, 2f                 \n"
		
        // 3. Восстановить стек нового потока
        "ldr   r1, [r0, #16]          \n" // TSP
        "msr   psp, r1                \n"

        // Адрес current_thread->TR
        "add   r2, r0, #28            \n"

        // Восстановить R4-R11
        "ldmia r2, {r4-r11}           \n"

        // Восстановить EXC_RETURN
        "ldr   lr, [r2, #32]          \n"
		
		"2:                        \n"
		"ldr   r3, =switch_pending \n" // r3 = адрес флага
		"movs  r2, #0              \n" // r2 = 0
		"strb  r2, [r3]            \n" // планировщик снова разрешён
		"bx    lr                  \n"
    );
}

extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
    if (current_thread) {
        current_thread->TCpuT += 1000;                    // 1 мс в мкс
        if (current_thread->TSTATE != READY) quantum_left = 0;
    }
    if (quantum_left) quantum_left--;
    if (quantum_left == 0) scheduler();
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
	if (switch_pending) return NULL;
	if (current_thread && current_thread->TSTATE == RUNNING) current_thread->TSTATE = READY;
	
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

						quantum_left = 1;
						current_thread->TSTATE = RUNNING;
						
						if (last_thread != current_thread)
						{	
							switch_pending = 1;
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
	
ProcessControlBlock* createProcess(void)
{
	if (ProcessCount >= MAX_PROCESS_IN_LIST)
	{
		return NULL;
	}
	if (PIDCount == 65535)
	{
		PIDCount = 1;
	}
	for (uint16_t pcPID = PIDCount+1; pcPID <= 65535; pcPID++)  // PIDCount это последний созданый PID соответственно PIDCount+1 это PID создоваемого процесса.
	{
		if (pcPID == PIDCount)
		{
			return NULL;
		}
		if (((PIDFree[pcPID / 8] >> (pcPID % 8)) & 1u) == 0)
		{
			PIDCount = pcPID;
			break;
		} 
		if (pcPID == 65535)
		{
			pcPID = 0;
		} 
			// bitmap[index / 8] |= (uint8_t)(1u << (index % 8)); // Записать 1
			// bitmap[index / 8] &= (uint8_t)~(1u << (index % 8)); // Записать 0
			// uint8_t value = (bitmap[index / 8] >> (index % 8)) & 1u; // Прочитать бит
	}
	
	PIDFree[PIDCount / 8] |= (uint8_t)(1u << (PIDCount % 8));
	
	return NULL;
}