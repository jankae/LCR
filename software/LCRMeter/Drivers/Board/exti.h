#ifndef EXTI_H_
#define EXTI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm.h"

typedef enum {
	EXTI_TYPE_FALLING,
	EXTI_TYPE_RISING,
	EXTI_TYPE_RISING_FALLING,
} exti_type_t;

typedef enum {
	EXTI_PULL_NONE,
	EXTI_PULL_DOWN,
	EXTI_PULL_UP,
} exti_pull_t;

typedef enum {
	EXTI_RES_OK,
	EXTI_RES_ERROR,
} exti_result_t;

typedef void(*exti_callback_t)(void*);

void exti_init();
exti_result_t exti_set_callback(GPIO_TypeDef *gpio, uint16_t pin, exti_type_t type,
		exti_pull_t pull, exti_callback_t cb, void *ptr);
exti_result_t exti_clear_callback(GPIO_TypeDef *gpio, uint16_t pin);
void exti_get_callback(GPIO_TypeDef *gpio, uint16_t pin, exti_callback_t *cb,
		void **ptr);

#ifdef __cplusplus
}
#endif

#endif
