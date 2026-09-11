#include <stdint.h>
#include "stm32h5xx_hal.h"
#include "STM32FreeRTOS.h"

int main(void) {
    HAL_init();

    vTaskStartScheduler();

    while (1) {
    }
}