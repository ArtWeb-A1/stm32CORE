#include "drivGIPO.h"
#include "stm32h5xx_hal.h"

bool clockIsEnabled = false;

void clockEnable() {
	if clockIsEnabled == false {
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOD_CLK_ENABLE();
		__HAL_RCC_GPIOE_CLK_ENABLE();
	}
	clockIsEnabled = true;
}

void sysCallBeginGIPO() {
	clockEnable();
}

void BeginGIPO(uint16_t pin, uint32_t mode, uint32_t pull = GPIO_NOPULL) {
	clockEnable();
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = mode;
    GPIO_InitStruct.Pull  = pull;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}