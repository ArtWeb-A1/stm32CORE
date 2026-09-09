#ifndef STM32_FREERTOS_CONFIG_H
#define STM32_FREERTOS_CONFIG_H

// 1. Сначала подключаем дефолтные настройки ядра
#include "FreeRTOSConfig_Default.h"

// 2. Переопределяем параметры под MPU и STM32H5 (16 регионов)
#undef configENABLE_MPU
#define configENABLE_MPU                        1

#undef configTOTAL_MPU_REGIONS
#define configTOTAL_MPU_REGIONS                16 // У STM32H562 16 регионов MPU

#endif /* STM32_FREERTOS_CONFIG_H */