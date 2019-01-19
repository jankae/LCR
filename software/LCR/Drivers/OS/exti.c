#include "exti.h"

#include "log.h"

#define EXTI_MAX_ENTRIES		16

typedef struct {
	GPIO_TypeDef *gpio;
	exti_callback_t cb;
	void *ptr;
} callback_entry_t;

static callback_entry_t entries[EXTI_MAX_ENTRIES];
static uint8_t initialized = 0;

void exti_init() {
	HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
	HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
	HAL_NVIC_SetPriority(EXTI2_TSC_IRQn, 5, 0);
	HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
	HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
	HAL_NVIC_EnableIRQ(EXTI2_TSC_IRQn);
	HAL_NVIC_EnableIRQ(EXTI3_IRQn);
	HAL_NVIC_EnableIRQ(EXTI4_IRQn);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

	for (uint8_t i = 0; i < EXTI_MAX_ENTRIES; i++) {
		entries[i].gpio = NULL;
		entries[i].cb = NULL;
		entries[i].ptr = NULL;
	}

	initialized = 1;
}

exti_result_t exti_set_callback(GPIO_TypeDef *gpio, uint16_t pin, exti_type_t type,
		exti_pull_t pull, exti_callback_t cb, void *ptr) {
	if (!initialized) {
		exti_init();
	}
	ASSERT(pin != 0);
	ASSERT(gpio != NULL);

	uint8_t index = 31 - __builtin_clz(pin);
	ASSERT(pin < 1 << EXTI_MAX_ENTRIES);

	if (entries[index].gpio && entries[index].gpio != gpio) {
		LOG(Log_Exti, LevelError,
				"Unable to set callback for pin %d, another GPIO already active: %p",
				pin, entries[index].gpio);
		return EXTI_RES_ERROR;
	}

	GPIO_InitTypeDef g;
	g.Pin = pin;
	switch (type) {
	case EXTI_TYPE_FALLING:
		g.Mode = GPIO_MODE_IT_FALLING;
		break;
	case EXTI_TYPE_RISING:
		g.Mode = GPIO_MODE_IT_RISING;
		break;
	case EXTI_TYPE_RISING_FALLING:
		g.Mode = GPIO_MODE_IT_RISING_FALLING;
		break;
	}
	switch (pull) {
	case EXTI_PULL_NONE:
		g.Pull = GPIO_NOPULL;
		break;
	case EXTI_PULL_UP:
		g.Pull = GPIO_PULLUP;
		break;
	case EXTI_PULL_DOWN:
		g.Pull = GPIO_PULLDOWN;
		break;
	}
	HAL_GPIO_Init(gpio, &g);

	entries[index].gpio = gpio;
	entries[index].cb = cb;
	entries[index].ptr = ptr;
	LOG(Log_Exti, LevelInfo, "Callback set for pin %d, GPIO %p", index,
			entries[index].gpio);
	return EXTI_RES_OK;
}

exti_result_t exti_clear_callback(GPIO_TypeDef *gpio, uint16_t pin) {
	if (!initialized) {
		exti_init();
	}
	ASSERT(pin != 0);
	uint8_t index = 31 - __builtin_clz(pin);
	if (entries[index].gpio) {
		if (gpio != entries[index].gpio) {
			LOG(Log_Exti, LevelError,
					"Unable to clear callback for pin %d, GPIO mismatch (expected %p, got %p)",
					pin, entries[index].gpio, gpio);
			return EXTI_RES_ERROR;
		}
	}

	entries[index].gpio = NULL;
	entries[index].cb = NULL;
	entries[index].ptr = NULL;
	LOG(Log_Exti, LevelInfo, "Callback cleared for pin %d, GPIO %p", index,
			gpio);
	return EXTI_RES_OK;
}

static inline void ExtiHandler(uint16_t source) {
	const uint32_t mask = 1 << source;
	if (__HAL_GPIO_EXTI_GET_IT(mask) != RESET) {
		__HAL_GPIO_EXTI_CLEAR_IT(mask);
		if (entries[source].cb) {
			entries[source].cb(entries[source].ptr);
		}
	}
}

void EXTI0_IRQHandler(void) {
	ExtiHandler(0);
}

void EXTI1_IRQHandler(void) {
	ExtiHandler(1);
}

void EXTI2_IRQHandler(void) {
	ExtiHandler(2);
}

void EXTI3_IRQHandler(void) {
	ExtiHandler(3);
}

void EXTI4_IRQHandler(void) {
	ExtiHandler(4);
}

void EXTI9_5_IRQHandler(void) {
	ExtiHandler(5);
	ExtiHandler(6);
	ExtiHandler(7);
	ExtiHandler(8);
	ExtiHandler(9);
}

void EXTI15_10_IRQHandler(void) {
	ExtiHandler(10);
	ExtiHandler(11);
	ExtiHandler(12);
	ExtiHandler(13);
	ExtiHandler(14);
	ExtiHandler(15);
}
