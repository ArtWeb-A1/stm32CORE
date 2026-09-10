#ifndef STM32_FREERTOS_CONFIG_H
#define STM32_FREERTOS_CONFIG_H

// 1. Ñíà÷àëà ïîäêëþ÷àåì äåôîëòíûå íàñòðîéêè ÿäðà
#include "FreeRTOSConfig_Default.h"

// 2. Ïåðåîïðåäåëÿåì ïàðàìåòðû ïîä MPU è STM32H5 (16 ðåãèîíîâ)
#undef configENABLE_MPU
#define configENABLE_MPU                        1

#undef configTOTAL_MPU_REGIONS
#define configTOTAL_MPU_REGIONS                16 // Ó STM32H562 16 ðåãèîíîâ MPU

#endif /* STM32_FREERTOS_CONFIG_H */
