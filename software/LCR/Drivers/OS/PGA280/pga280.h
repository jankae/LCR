#ifndef OS_PGA280_PGA280_H_
#define OS_PGA280_PGA280_H_

#include "stm.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SPI_HandleTypeDef *spi;
	GPIO_TypeDef *CSgpio;
	uint16_t CSpin;
} pga280_t;

typedef enum {
	PGA280_RES_OK,
	PGA280_RES_ERROR,
} pga280_result_t;

typedef enum {
	PGA280_GAIN_1_8 = 0,
	PGA280_GAIN_1_4 = 1,
	PGA280_GAIN_1_2 = 2,
	PGA280_GAIN_1 = 3,
	PGA280_GAIN_2 = 4,
	PGA280_GAIN_4 = 5,
	PGA280_GAIN_8 = 6,
	PGA280_GAIN_16 = 7,
	PGA280_GAIN_32 = 8,
	PGA280_GAIN_64 = 9,
	PGA280_GAIN_128 = 10,
} pga280_gain_t;

typedef enum {
	PGA280_SW_A1 = 0x40,
	PGA280_SW_A2 = 0x20,
	PGA280_SW_B1 = 0x10,
	PGA280_SW_B2 = 0x08,
	PGA280_SW_C1 = 0x04,
	PGA280_SW_C2 = 0x02,
	PGA280_SW_D12 = 0x01,
	PGA280_SW_F1 = 0x88,
	PGA280_SW_F2 = 0x84,
	PGA280_SW_G1 = 0x82,
	PGA280_SW_G2 = 0x81,
} pga280_switch_t;

typedef enum {
	PGA280_GPIO0 = 0x01,
	PGA280_GPIO1 = 0x02,
	PGA280_GPIO2 = 0x04,
	PGA280_GPIO3 = 0x08,
	PGA280_GPIO4 = 0x10,
	PGA280_GPIO5 = 0x20,
	PGA280_GPIO6 = 0x40,
} pga280_gpio_t;

typedef enum {
	PGA280_INPUT1_DIFF,
	PGA280_INPUT2_DIFF,
	PGA280_INP1_SINGLE,
	PGA280_INP2_SINGLE,
	PGA280_INN1_SINGLE,
	PGA280_INN2_SINGLE
} pga280_input_t;

pga280_result_t pga280_init(pga280_t *p);
pga280_result_t pga280_set_gain(pga280_t *p, pga280_gain_t gain, bool multBy1_375);
uint8_t pga280_get_error(pga280_t *p);
pga280_result_t pga280_clear_error(pga280_t *p, uint8_t error);
pga280_result_t pga280_set_switch(pga280_t *p, pga280_switch_t sw, bool closed);
pga280_result_t pga280_select_input(pga280_t *p, pga280_input_t input);

#ifdef __cplusplus
}
#endif

#endif
