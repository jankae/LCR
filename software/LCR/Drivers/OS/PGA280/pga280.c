#include "pga280.h"
#include "log.h"

static void write_register(pga280_t *p, uint8_t reg, uint8_t value) {
	p->CSgpio->BSRR = p->CSpin << 16;
	uint8_t data[2];
	data[0] = 0x40 | (reg & 0x0F);
	data[1] = value;
	HAL_SPI_Transmit(p->spi, data, 2, 1000);
	p->CSgpio->BSRR = p->CSpin;
}

static uint8_t read_register(pga280_t *p, uint8_t reg) {
	p->CSgpio->BSRR = p->CSpin << 16;
	uint8_t write[2];
	write[0] = 0x80 | (reg & 0x0F);
	write[1] = 0;
	uint8_t read[2];
	HAL_SPI_TransmitReceive(p->spi, write, read, 2, 1000);
	p->CSgpio->BSRR = p->CSpin;
	return read[1];
}

static void set_bits(pga280_t *p, uint8_t reg, uint8_t mask) {
	uint8_t val = read_register(p, reg);
	val |= mask;
	write_register(p, reg, val);
}

static void clear_bits(pga280_t *p, uint8_t reg, uint8_t mask) {
	uint8_t val = read_register(p, reg);
	val &= ~mask;
	write_register(p, reg, val);
}

static void modify_bits(pga280_t *p, uint8_t reg, uint8_t mask, uint8_t value) {
	uint8_t val = read_register(p, reg);
	val &= ~mask;
	val |= (mask & value);
	write_register(p, reg, val);
}

pga280_result_t pga280_init(pga280_t *p) {
	// initialize GPIO pins
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	gpio.Pin = p->CSpin;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(p->CSgpio, &gpio);

	// Set CS pin high
	p->CSgpio->BSRR = p->CSpin;

	// perform software reset
	write_register(p, 0x01, 0x01);

	// check functionality be comparing register content with reset value
	uint8_t buftim = read_register(p, 0x03);
	if (buftim != 0x19) {
		LOG(Log_PGA280, LevelError,
				"Invalid BUFTIM value (got 0x%02x, expected 0x19), init failed",
				buftim);
		return PGA280_RES_ERROR;
	}

	LOG(Log_PGA280, LevelInfo, "Initialized");
	return PGA280_RES_OK;
}

pga280_result_t pga280_set_gain(pga280_t *p, pga280_gain_t gain,
		bool multBy1_375) {
	uint8_t value = ((int) gain & 0x0F) << 3;
	if (multBy1_375) {
		value |= 0x80;
	}
	modify_bits(p, 0x00, 0xF8, value);
	return PGA280_RES_OK;
}

uint8_t pga280_get_error(pga280_t *p) {
	return read_register(p, 0x04);
}
pga280_result_t pga280_clear_error(pga280_t *p, uint8_t error) {
	write_register(p, 0x04, error);
	return PGA280_RES_OK;
}

pga280_result_t pga280_set_switch(pga280_t *p, pga280_switch_t sw, bool closed) {
	uint8_t reg = 0x06;
	uint8_t swval = (int) sw;
	if (swval & 0x80) {
		reg = 0x07;
		swval &= ~0x80;
	}
	uint8_t state = closed ? 0xFF : 0x00;
	modify_bits(p, reg, swval, state);
	return PGA280_RES_OK;
}

pga280_result_t pga280_select_input(pga280_t *p, pga280_input_t input) {
	switch(input) {
	case PGA280_INPUT1_DIFF:
		pga280_set_switch(p, PGA280_SW_B1, false);
		pga280_set_switch(p, PGA280_SW_B2, false);
		pga280_set_switch(p, PGA280_SW_C1, false);
		pga280_set_switch(p, PGA280_SW_C2, false);
		pga280_set_switch(p, PGA280_SW_A1, true);
		pga280_set_switch(p, PGA280_SW_A2, true);
		break;
	case PGA280_INPUT2_DIFF:
		pga280_set_switch(p, PGA280_SW_A1, false);
		pga280_set_switch(p, PGA280_SW_A2, false);
		pga280_set_switch(p, PGA280_SW_C1, false);
		pga280_set_switch(p, PGA280_SW_C2, false);
		pga280_set_switch(p, PGA280_SW_B1, true);
		pga280_set_switch(p, PGA280_SW_B2, true);
		break;
	case PGA280_INP1_SINGLE:
		pga280_set_switch(p, PGA280_SW_B1, false);
		pga280_set_switch(p, PGA280_SW_B2, false);
		pga280_set_switch(p, PGA280_SW_C1, false);
		pga280_set_switch(p, PGA280_SW_A2, false);
		pga280_set_switch(p, PGA280_SW_C2, true);
		pga280_set_switch(p, PGA280_SW_A1, true);
		break;
	case PGA280_INP2_SINGLE:
		pga280_set_switch(p, PGA280_SW_A1, false);
		pga280_set_switch(p, PGA280_SW_A2, false);
		pga280_set_switch(p, PGA280_SW_B2, false);
		pga280_set_switch(p, PGA280_SW_C1, false);
		pga280_set_switch(p, PGA280_SW_B1, true);
		pga280_set_switch(p, PGA280_SW_C2, true);
		break;
	case PGA280_INN1_SINGLE:
		pga280_set_switch(p, PGA280_SW_B1, false);
		pga280_set_switch(p, PGA280_SW_B2, false);
		pga280_set_switch(p, PGA280_SW_A1, false);
		pga280_set_switch(p, PGA280_SW_C2, false);
		pga280_set_switch(p, PGA280_SW_C1, true);
		pga280_set_switch(p, PGA280_SW_A2, true);
		break;
	case PGA280_INN2_SINGLE:
		pga280_set_switch(p, PGA280_SW_A1, false);
		pga280_set_switch(p, PGA280_SW_A2, false);
		pga280_set_switch(p, PGA280_SW_B1, false);
		pga280_set_switch(p, PGA280_SW_C2, false);
		pga280_set_switch(p, PGA280_SW_C1, true);
		pga280_set_switch(p, PGA280_SW_B2, true);
		break;
	}
	return PGA280_RES_OK;
}
