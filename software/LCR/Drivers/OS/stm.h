#ifndef IOX_STM_H_
#define IOX_STM_H_

#ifdef __cplusplus
extern "C" {
#endif

// Project dependent stm includes
#include "stm32f3xx.h"
#include "stm32f3xx_hal.h"

static inline uint8_t stm_in_interrupt() {
	return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

#ifdef HAL_RTC_MODULE_ENABLED
int stm_set_rtc(uint32_t timestamp);
uint32_t stm_get_rtc();
#endif

#ifdef __cplusplus
}
#endif

#endif
