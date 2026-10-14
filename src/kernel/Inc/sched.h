#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <stdint.h>
#include <stddef.h>
#include "stm32h5xx_hal.h"

#define MAX_TIDs_PER_PROCESS 4U
#define MAX_MPU_REGIONS      12U
#define MAX_PROCESS_IN_LIST  16U
#define MAX_THREADS_IN_LIST  64U 

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

extern ProcessControlBlock ProcessList[MAX_PROCESS_IN_LIST];
extern ThreadControlBlock ThreadList[MAX_THREADS_IN_LIST];
extern MPURegionConfig     MPU_regions[MAX_MPU_REGIONS];

extern uint8_t PIDFree[65536 / 8];

ThreadControlBlock *scheduler(void);
ProcessControlBlock* createProcess(void);

#endif 