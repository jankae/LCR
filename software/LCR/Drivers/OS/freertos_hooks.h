#ifndef IOX_FREERTOS_HOOKS_H_
#define IOX_FREERTOS_HOOKS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"

#define HARDFAULT_HOOK

#if configUSE_TICKLESS_IDLE == 1
#define RTC_FREQUENCY					37000
void pd_prevent_stop();
void pd_allow_stop();
#define PD_PREVENT_STOP()				pd_prevent_stop()
#define PD_ALLOW_STOP()					pd_allow_stop()
#else
#define PD_PREVENT_STOP()
#define PD_ALLOW_STOP()
#endif

// additional memory allocation functions
void *pvPortCalloc(size_t num, size_t xSize);

#if configUSE_TRACE_FACILITY == 1
int freertos_print_task_overview();
#endif

#ifdef __cplusplus
}
#endif

#endif
