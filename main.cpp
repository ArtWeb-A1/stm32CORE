// #include <stdint.h>
// #include "sched.h"
// #include "stm32h5xx_hal.h"

// // ==================== [Variables] ==================== //





	// // ==================== [Structures] ==================== //




	// // ==================== [Constants] ==================== //




// // ==================== [Call func prototypes] ==================== //



// // ==================== [Main function] ==================== //

// int main(void) 
// {
    // HAL_Init();
   // // UART должен быть предварительно настроен
    // HAL_UART_Transmit(&huart3, (uint8_t*)"Restart!\r\n", 9, HAL_MAX_DELAY);
 	
    // while (1) {
    // }
// }

// // ==================== [Another functions] ==================== //


#include <cstring>
#include "stm32h5xx.h"

int main()
{
    // PendSV - самый низкий приоритет: переключение контекста не должно прерывать
    // другие обработчики прерываний (и не должно быть прервано ими посреди смены PSP).
    NVIC_SetPriority(PendSV_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);

    // HAL_Init();

    while (1)
    {
    }
}