#include "ad5940.h"

#include "log.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <util.h>

#define AD_IDENT_REGVAL			0x4144
#define CHIP_ID_REGVAL			0x5502

#define Log_AD5940		(LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

typedef enum {
	AD5940_SPICMD_SETADDR = 		0x20,
	AD5940_SPICMD_READREG = 		0x6D,
	AD5940_SPICMD_WRITEREG = 		0x2D,
	AD5940_SPICMD_READFIFO =		0x5F,
} ad5940_spicmd_t;

typedef struct {
	ad5940_tia_gain_t gain;
	uint32_t factor;
} ad5940_lptia_gain_factor_t;

static const ad5940_lptia_gain_factor_t lptia_gain_factors[] = {
	{AD5940_TIA_GAIN_OPEN, 0},
	{AD5940_TIA_GAIN_200, 200},
	{AD5940_TIA_GAIN_1K, 1000},
	{AD5940_TIA_GAIN_2K, 2000},
	{AD5940_TIA_GAIN_3K, 3000},
	{AD5940_TIA_GAIN_4K, 4000},
	{AD5940_TIA_GAIN_6K, 6000},
	{AD5940_TIA_GAIN_8K, 8000},
	{AD5940_TIA_GAIN_10K, 10000},
	{AD5940_TIA_GAIN_12K, 12000},
	{AD5940_TIA_GAIN_16K, 16000},
	{AD5940_TIA_GAIN_20K, 20000},
	{AD5940_TIA_GAIN_24K, 24000},
	{AD5940_TIA_GAIN_30K, 30000},
	{AD5940_TIA_GAIN_32K, 32000},
	{AD5940_TIA_GAIN_40K, 40000}, // 40K range seems to have less accuracy, skip
	{AD5940_TIA_GAIN_48K, 48000},
	{AD5940_TIA_GAIN_64K, 64000},
	{AD5940_TIA_GAIN_85K, 85000},
	{AD5940_TIA_GAIN_96K, 96000},
	{AD5940_TIA_GAIN_100K, 100000},
	{AD5940_TIA_GAIN_120K, 120000},
	{AD5940_TIA_GAIN_128K, 128000},
	{AD5940_TIA_GAIN_160K, 160000},
	{AD5940_TIA_GAIN_196K, 196000},
	{AD5940_TIA_GAIN_256K, 256000},
	{AD5940_TIA_GAIN_512K, 512000},
};

static const uint8_t num_lptia_gain_factors = sizeof(lptia_gain_factors)
		/ sizeof(lptia_gain_factors[0]);

typedef struct {
	ad5940_hsrtia_t gain;
	uint32_t factor;
} ad5940_hstia_gain_factor_t;

static const ad5940_hstia_gain_factor_t hstia_gain_factors[] = {
	{AD5940_HSRTIA_OPEN, 0},
	{AD5940_HSRTIA_200, 200},
	{AD5940_HSRTIA_1K, 1000},
	{AD5940_HSRTIA_5K, 5000},
	{AD5940_HSRTIA_10K, 10000},
	{AD5940_HSRTIA_20K, 20000},
	{AD5940_HSRTIA_40K, 40000},
	{AD5940_HSRTIA_80K, 80000},
//	{AD5940_HSRTIA_160K, 160000}, // Experiments show that HSTIA isn't able to maintain regulation with highest gain
};

static const uint8_t num_hstia_gain_factors = sizeof(hstia_gain_factors)
		/ sizeof(hstia_gain_factors[0]);

typedef struct {
	ad5940_pga_gain_t gain;
	uint32_t factor;
} ad5940_pga_gain_factor_t;

static const ad5940_pga_gain_factor_t pga_gain_factors[] = {
	{AD5940_PGA_GAIN_1, 10},
	{AD5940_PGA_GAIN_1_5, 15},
	{AD5940_PGA_GAIN_2, 20},
	{AD5940_PGA_GAIN_4, 40},
	{AD5940_PGA_GAIN_9, 90},
};

static const uint8_t num_pga_gain_factors = sizeof(pga_gain_factors)
		/ sizeof(pga_gain_factors[0]);

void ad5940_take_mutex(ad5940_t *a) {
#ifdef AD5940_USE_MUTEX
	xSemaphoreTakeRecursive(a->access_mutex, portMAX_DELAY);
#endif
}

void ad5940_release_mutex(ad5940_t *a) {
#ifdef AD5940_USE_MUTEX
	xSemaphoreGiveRecursive(a->access_mutex);
#endif
}

static void cs_low(ad5940_t *a) {
	a->CSport->BSRR = a->CSpin << 16;
}
static void cs_high(ad5940_t *a) {
	a->CSport->BSRR = a->CSpin;
}

static uint8_t get_reg_length(ad5940_reg_t reg) {
	if (reg >= 0x1000 && reg <= 0x3014) {
		return 4;
	} else {
		return 2;
	}
}

void ad5940_write_reg(ad5940_t *a, ad5940_reg_t reg, uint32_t val) {
#ifdef AD5940_USE_SPI_MUTEX
	xSemaphoreTake(AD5940_SPI_MUTEX, portMAX_DELAY);
#endif
	// set register address
	cs_low(a);
	{
		uint8_t data[3] = { AD5940_SPICMD_SETADDR, (uint32_t) reg >> 8,
				(uint32_t) reg & 0xFF };
		HAL_SPI_Transmit(a->spi, data, 3, 100);
	}
	cs_high(a);
	asm volatile("NOP");
	cs_low(a);
	{
		uint8_t reglength = get_reg_length(reg);
		uint8_t data[5];
		data[0] = AD5940_SPICMD_WRITEREG;
		if (reglength == 2) {
			data[1] = (val >> 8) & 0xFF;
			data[2] = val & 0xFF;
		} else {
			data[1] = (val >> 24) & 0xFF;
			data[2] = (val >> 16) & 0xFF;
			data[3] = (val >> 8) & 0xFF;
			data[4] = val & 0xFF;
		}
		HAL_SPI_Transmit(a->spi, data, reglength + 1, 100);
	}
	cs_high(a);
#ifdef AD5940_USE_SPI_MUTEX
	xSemaphoreGive(AD5940_SPI_MUTEX);
#endif
}

uint32_t ad5940_read_reg(ad5940_t *a, ad5940_reg_t reg) {
#ifdef AD5940_USE_SPI_MUTEX
	xSemaphoreTake(AD5940_SPI_MUTEX, portMAX_DELAY);
#endif
	// set register address
	cs_low(a);
	{
		uint8_t data[3] = { AD5940_SPICMD_SETADDR, (uint32_t) reg >> 8,
				(uint32_t) reg & 0xFF };
		HAL_SPI_Transmit(a->spi, data, 3, 100);
	}
	cs_high(a);
	asm volatile("NOP");
	cs_low(a);
	uint32_t ret = 0;
	{
		uint8_t reglength = get_reg_length(reg);
		uint8_t send[6] = { AD5940_SPICMD_READREG, 0, 0, 0, 0, 0 };
		uint8_t rec[6];
		HAL_SPI_TransmitReceive(a->spi, send, rec, reglength + 2, 100);
		if (reglength == 2) {
			ret = (uint32_t) (rec[2]) << 8 | (uint32_t) (rec[3]);
		} else {
			ret = (uint32_t) (rec[2]) << 24 | (uint32_t) (rec[3]) << 16
					| (uint32_t) (rec[4]) << 8 | (uint32_t) (rec[5]);
		}
	}
	cs_high(a);
#ifdef AD5940_USE_SPI_MUTEX
	xSemaphoreGive(AD5940_SPI_MUTEX);
#endif
	return ret;
}

void ad5940_set_bits(ad5940_t *a, ad5940_reg_t reg, uint32_t bits) {
	uint32_t val = ad5940_read_reg(a, reg);
	val |= bits;
	ad5940_write_reg(a, reg, val);
}

void ad5940_clear_bits(ad5940_t *a, ad5940_reg_t reg, uint32_t bits) {
	uint32_t val = ad5940_read_reg(a, reg);
	val &= ~bits;
	ad5940_write_reg(a, reg, val);
}

void ad5940_modify_reg(ad5940_t *a, ad5940_reg_t reg, uint32_t val,
		uint32_t mask) {
	uint32_t old = ad5940_read_reg(a, reg);
	old &= ~mask;
	old |= val & mask;
	ad5940_write_reg(a, reg, old);
}


ad5940_result_t ad5940_init(ad5940_t *a) {
#ifdef AD5940_USE_MUTEX
	a->access_mutex = xSemaphoreCreateMutexStatic(&a->xMutHardwareAccess);
#endif
	if(!a->CSport) {
		LOG(Log_AD5940, LevelCrit, "Null pointer given for CS port");
		return AD5940_RES_ERROR;
	}
	// initialize CS pin
	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
#if defined(GPIO_SPEED_FREQ_VERY_HIGH)
	gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
#elif defined(GPIO_SPEED_FREQ_HIGH)
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
#else
#error GPIO_SPEED_HIGH not defined, probably slightly different macro name for this mcu
#endif
	gpio.Pin = a->CSpin;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(a->CSport, &gpio);
	cs_high(a);

	a->autoranging = true;

	// check identification registers
	ad5940_read_reg(a, AD5940_REG_ADIID); // first SPI read seems to fail, do dummy read
	uint32_t AD_id = ad5940_read_reg(a, AD5940_REG_ADIID);
	uint32_t chip_id = ad5940_read_reg(a, AD5940_REG_CHIPID);
	if (AD_id != AD_IDENT_REGVAL || chip_id != CHIP_ID_REGVAL) {
		LOG(Log_AD5940, LevelError,
				"Chip identification mismatch: 0x%04x!=0x%04x/0x%04x!=0x%04x",
				AD_id, AD_IDENT_REGVAL, chip_id, CHIP_ID_REGVAL);
		return AD5940_RES_ERROR;
	}

	// initialization from AD example code
	const struct {
		uint16_t reg_addr;
		uint32_t reg_data;
	} RegTable[] = {
		{0x0908, 0x02c9}, // ? (Watchdog disable
		{0x0c08, 0x206C}, // ?
		{0x21F0, 0x0010}, // ADC repeat 1 conversion
		{0x0410, 0x02c9}, // CMDDATACON, command STREAM memory mode, 2kB memory, data FIFO mode ?, 4kb data
		{0x0A28, 0x0009}, // EI2CON, bus interrupt enable, falling edge
		{0x238c, 0x0104}, // ADCBUFCON, disable offset calibration buffers
		{0x0a04, 0x4859}, // unlock power reg (Key1)
		{0x0a04, 0xF27B}, // unlock power reg (Key2)
		{0x0a00, 0x8009}, // PWRMOD, enable RAM retention, enable sequencer autosleep, active mode
		{0x22F0, 0x0000}, // PMBW, ?
		{0x2230, 0xDE87A5AF}, // Unlock calibration data register
		{0x2250, 0x103F}, // ?
		{0x22B0, 0x203C}, // ?
		{0x2230, 0xDE87A5A0}, // Lock calibration data register
	};
	for (uint8_t i = 0; i < sizeof(RegTable) / sizeof(RegTable[0]); i++) {
		ad5940_write_reg(a, RegTable[i].reg_addr, RegTable[i].reg_data);
	}

	// enable low power DAC
	ad5940_clear_bits(a, AD5940_REG_LPDACCON0, 0x02);
	ad5940_set_bits(a, AD5940_REG_LPDACCON0, 0x01);

	ad5940_write_reg(a, AD5940_REG_ADCCON, 0x0000);

	// route DAC outputs to Vbias/Vzero pins for debugging
	ad5940_set_bits(a, AD5940_REG_LPDACSW0, 0x00E);

	ad5940_set_vzero(a, 2000);
	ad5940_set_TIA_lowpass(a, AD5940_TIA_RF_20K);
	ad5940_set_PGA_gain(a, AD5940_PGA_GAIN_4);
	ad5940_set_TIA_load(a, AD5940_TIA_RL_100);
	ad5940_set_TIA_gain(a, ad5940_tia_gain_default);

	// enable sinc2 50/60Hz notch
	ad5940_set_bits(a, AD5940_REG_AFECON, 0x00010000);

	// enable statistics, 128 samples
	ad5940_set_bits(a, AD5940_REG_STATSCON, 0x0001);

	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_electrode(ad5940_t *a, ad5940_electrode_mode_t e) {
	switch(e) {
	case AD5940_ELECTRODE_2WIRE:
//		// enable low power DAC
//		ad5940_clear_bits(a, AD5940_REG_LPDACCON0, 0x02);
//		ad5940_set_bits(a, AD5940_REG_LPDACCON0, 0x01);
		// close SW4 of low power DAC
		ad5940_set_bits(a, AD5940_REG_LPDACSW0, 0x30);
		// close SW2 and SW8 of low power TIA (low power DAC buffer mode + output enable)
		// disconnect reference electrode (open SW4 and SW3)
		ad5940_modify_reg(a, AD5940_REG_LPTIASW0, 0x0104, 0x011C);
		// power up PA and TIA
		ad5940_clear_bits(a, AD5940_REG_LPTIACON0, 0x03);
		break;
	case AD5940_ELECTRODE_3WIRE:
//		// enable low power DAC
//		ad5940_clear_bits(a, AD5940_REG_LPDACCON0, 0x02);
//		ad5940_set_bits(a, AD5940_REG_LPDACCON0, 0x01);
		// close SW4 of low power DAC
		ad5940_set_bits(a, AD5940_REG_LPDACSW0, 0x30);
		// close SW2, SW3 and SW4 (output enable + feedback from reference electrode)
		// disconnect buffer for PA (open SW8)
		ad5940_modify_reg(a, AD5940_REG_LPTIASW0, 0x001C, 0x011C);
		// power up PA and TIA
		ad5940_clear_bits(a, AD5940_REG_LPTIACON0, 0x03);
		break;
	case AD5940_ELECTRODE_DISCONNECTED:
		// power down the PA and LPTIA
		ad5940_set_bits(a, AD5940_REG_LPTIACON0, 0x03);
		// open all LPTIA switches
		ad5940_write_reg(a, AD5940_REG_LPTIASW0, 0x0000);
		// close SW2 and SW3 of low power DAC
		ad5940_write_reg(a, AD5940_REG_LPDACSW0, 0x000C);
		// Cutting power to the LPDAC does something weird and breaks measurement functionality
		// afterwards. Keep DAC powered on, it doesn't really matter because the PA is powered down
		//ad5940_set_bits(a, AD5940_REG_LPDACCON0, 0x02);
		break;
	}
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_vzero(ad5940_t *a, uint16_t mV) {
	int32_t val = ((int32_t) (mV - 200) * 1000 + 17187) / 34375;
	if (val < 0 || val > 63) {
		LOG(Log_AD5940, LevelWarn,
				"Unable to set requested Vzero of %dmV (DAC range is 0 to 63, would need %d)",
				mV, val);
		return AD5940_RES_ERROR;
	}
	a->vzero_code = val;
	uint32_t dac = (uint32_t) val << 12;
	ad5940_modify_reg(a, AD5940_REG_LPDACDAT0, dac, 0x0003F000);
	uint16_t actual = 200 + 34375 * val / 1000;
	LOG(Log_AD5940, LevelDebug, "Set Vzero to %umV (requested: %u)", actual,
			mV);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_vbias(ad5940_t *a, int16_t mV) {
	uint16_t vzero = 200 + 34375UL * a->vzero_code / 1000;
	int32_t dac_voltage = mV + vzero;
	int32_t val = ((int32_t) (dac_voltage - 200) * 1000 + 268) / 537;
	if (val < 0 || val > 4095) {
		LOG(Log_AD5940, LevelWarn,
				"Unable to set requested Vbias of %dmV (DAC range is 0 to 4095, would need %d, Vzero should be adjusted)",
				mV, val);
		return AD5940_RES_ERROR;
	}
	ad5940_modify_reg(a, AD5940_REG_LPDACDAT0, val, 0x00000FFF);
	int16_t actual = 200 + 537 * val / 1000 - vzero;
	LOG(Log_AD5940, LevelDebug, "Set Vbias to %dmV (requested: %d)", actual,
			mV);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_TIA_lowpass(ad5940_t *a, ad5940_tia_rf_t rf) {
	ad5940_modify_reg(a, AD5940_REG_LPTIACON0, rf, 0xE000);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_TIA_load(ad5940_t *a, ad5940_tia_rl_t rl) {
	ad5940_modify_reg(a, AD5940_REG_LPTIACON0, rl, 0x1C00);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_TIA_gain(ad5940_t *a, ad5940_tia_gain_t g) {
	ad5940_modify_reg(a, AD5940_REG_LPTIACON0, g, 0x03E0);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_get_TIA_gain(ad5940_t *a, ad5940_tia_gain_t *g) {
	*g = ad5940_read_reg(a, AD5940_REG_LPTIACON0) & 0x03E0;
	return AD5940_RES_OK;
}

static uint8_t LPTIA_gain_index(ad5940_tia_gain_t g) {
	for (uint8_t i = 0; i < num_lptia_gain_factors; i++) {
		if (lptia_gain_factors[i].gain == g) {
			return i;
		}
	}
	return 0;
}

uint32_t ad5940_LPTIA_gain_to_value(ad5940_tia_gain_t g) {
	return lptia_gain_factors[LPTIA_gain_index(g)].factor;
}

ad5940_tia_gain_t ad5940_value_to_LPTIA_gain(uint32_t value) {
	for (uint8_t i = 0; i < num_lptia_gain_factors; i++) {
		if (lptia_gain_factors[i].factor == value) {
			return lptia_gain_factors[i].gain;
		}
	}
	LOG(Log_AD5940, LevelError, "LPTIA gain with factor %lu not available", value);
	return 0;
}

ad5940_result_t ad5940_set_ADC_mux(ad5940_t *a, ad5940_adc_muxp_t p, ad5940_adc_muxn_t n) {
	ad5940_modify_reg(a, AD5940_REG_ADCCON, p | n, 0x1FFF);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_PGA_gain(ad5940_t *a, ad5940_pga_gain_t gain) {
	ad5940_modify_reg(a, AD5940_REG_ADCCON, gain, 0x00070000);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_get_PGA_gain(ad5940_t *a, ad5940_pga_gain_t *gain) {
	*gain = ad5940_read_reg(a, AD5940_REG_ADCCON) & 0x00070000;
	return AD5940_RES_OK;
}

static uint8_t PGA_gain_index(ad5940_pga_gain_t g) {
	for (uint8_t i = 0; i < num_pga_gain_factors; i++) {
		if (pga_gain_factors[i].gain == g) {
			return i;
		}
	}
	return 0;
}

uint8_t ad5940_PGA_gain_to_value10(ad5940_pga_gain_t g) {
	return pga_gain_factors[PGA_gain_index(g)].factor;
}

uint16_t ad5940_get_raw_ADC(ad5940_t *a) {
	return ad5940_read_reg(a, AD5940_REG_SINC2DAT);
}

ad5940_result_t ad5940_zero_ADC(ad5940_t *a) {
	// unlock and reset calibration register
	ad5940_write_reg(a, AD5940_REG_CALDATLOCK, 0xDE87A5AF);
	ad5940_write_reg(a, AD5940_REG_ADCOFFSETLPTIA, 0);

	// save current ADC MUX selection and connect both inputs to the same signal
	uint32_t adccon = ad5940_read_reg(a, AD5940_REG_ADCCON);
	ad5940_set_ADC_mux(a, AD5940_ADC_MUXP_LPTIAPLP, AD5940_ADC_MUXN_AIN4);
	// configure LPTIA in buffer mode, disconnecting it from the sense electrode
	ad5940_tia_gain_t g;
	ad5940_get_TIA_gain(a, &g);
	ad5940_set_TIA_gain(a, AD5940_TIA_GAIN_OPEN);
	ad5940_set_bits(a, AD5940_REG_LPTIASW0, 0x02A0);
	int32_t sum = 0;
#define N_SAMPLES		100
	for(uint16_t i = 0;i<N_SAMPLES;i++) {
		uint16_t raw = ad5940_read_reg(a, AD5940_REG_SINC2DAT);
		LOG(Log_AD5940, LevelDebug, "%d", raw);
		sum += raw;
		vTaskDelay(5);
	}
	sum /= N_SAMPLES;
	sum -= 32768;
	LOG(Log_AD5940, LevelDebug, "Offset LPTIA: %d", sum);

	// restore previous settings
	ad5940_clear_bits(a, AD5940_REG_LPTIASW0, 0x02A0);
	ad5940_set_TIA_gain(a, g);
	ad5940_write_reg(a, AD5940_REG_ADCCON, adccon);

	// update and lock calibration register
	ad5940_write_reg(a, AD5940_REG_ADCOFFSETLPTIA, -sum * 4);
	ad5940_write_reg(a, AD5940_REG_CALDATLOCK, 0x00000000);

	return AD5940_RES_OK;
}

ad5940_result_t ad5940_ADC_start(ad5940_t *a) {
	// enable ADC power
	ad5940_set_bits(a, AD5940_REG_AFECON, 0x0080);
	// start conversions
	ad5940_set_bits(a, AD5940_REG_AFECON, 0x0100);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_SINC2_OSR(ad5940_t *a, ad5940_sinc2_osr_t osr) {
	ad5940_modify_reg(a, AD5940_REG_ADCFILTERCON, ((uint16_t) osr) << 8, 0x0F00);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_ADC_stop(ad5940_t *a) {
	// stop conversions
	ad5940_clear_bits(a, AD5940_REG_AFECON, 0x0100);
	// disable ADC power
	ad5940_clear_bits(a, AD5940_REG_AFECON, 0x0080);
	return AD5940_RES_OK;
}

//ad5940_result_t ad5940_find_optimal_gain(ad5940_t *a) {
//#define LOW_THRESHOLD		14000
//#define HIGH_THRESHOLD		28500
//#define ADC_REFERENCE_uV 	1835000
//	int16_t raw;
//	ad5940_set_TIA_gain(a, AD5940_TIA_GAIN_200);
//	bool has_been_reduced = false;
//	do {
//		vTaskDelay(10);
//		raw = ad5940_get_raw_ADC(a) - 32768L;
//		LOG(Log_AD5940, LevelDebug, "Raw ADC: %d", raw);
//		if (abs(raw) < LOW_THRESHOLD) {
//			if (has_been_reduced) {
//				LOG(Log_AD5940, LevelDebug,
//						"Not increasing gain to prevent oscillation");
//				break;
//			}
//			ad5940_tia_gain_t g;
//			ad5940_get_TIA_gain(a, &g);
//			uint8_t fact = TIA_gain_index(g);
//			if(fact < num_tia_gain_factors - 1) {
//				fact++;
//				LOG(Log_AD5940, LevelDebug, "Increasing TIA gain to %lu", tia_gain_factors[fact].factor);
//				ad5940_set_TIA_gain(a, tia_gain_factors[fact].gain);
//			} else {
//				LOG(Log_AD5940, LevelDebug, "Would increase TIA gain but reached limit");
//				break;
//			}
//		} else if (abs(raw) > HIGH_THRESHOLD) {
//			ad5940_tia_gain_t g;
//			ad5940_get_TIA_gain(a, &g);
//			uint8_t fact = TIA_gain_index(g);
//			if(fact > 0) {
//				fact--;
//				has_been_reduced = true;
//				LOG(Log_AD5940, LevelDebug, "Decreasing TIA gain to %lu", tia_gain_factors[fact].factor);
//				ad5940_set_TIA_gain(a, tia_gain_factors[fact].gain);
//			} else {
//				LOG(Log_AD5940, LevelDebug, "Would decrease TIA gain but reached limit");
//				break;
//			}
//		} else {
//			LOG(Log_AD5940, LevelDebug, "Optimum TIA gain reached");
//			break;
//		}
//	} while (1);
//
//	return AD5940_RES_OK;
//}

#define ADC_REFERENCE_uV 	1835000

int32_t ad5940_raw_ADC_to_current(ad5940_t *a, int16_t raw) {
	// calculate current from raw ADC value
	ad5940_tia_gain_t g;
	ad5940_get_TIA_gain(a, &g);
	ad5940_pga_gain_t pga;
	ad5940_get_PGA_gain(a, &pga);
	uint32_t factor = ad5940_LPTIA_gain_to_value(g)
			* ad5940_PGA_gain_to_value10(pga) / 10;

	int64_t vol_adc_uv = (int64_t) raw * ADC_REFERENCE_uV / 32768;
	int32_t ret = vol_adc_uv * 1000 / factor;
//	LOG(Log_AD5940, LevelDebug, "Current: %ldnA", ret);
	return ret;
}

ad5940_result_t ad5940_measure_current(ad5940_t *a, int32_t *nA) {
	uint32_t sum = 0;
	for (uint8_t i = 0; i < 100; i++) {
		sum += ad5940_read_reg(a, AD5940_REG_SINC2DAT);
		vTaskDelay(5);
	}
	int16_t raw = sum / 100 - 32768L;

	// calculate current from raw ADC value
	*nA = ad5940_raw_ADC_to_current(a, raw);

	if (a->autoranging) {
		// check range
#define LOW_THRESHOLD		14000
#define HIGH_THRESHOLD		30000
		LOG(Log_AD5940, LevelDebug, "Raw ADC: %d", raw);
		if (abs(raw) < LOW_THRESHOLD) {
			ad5940_tia_gain_t g;
			ad5940_get_TIA_gain(a, &g);
			uint8_t fact = LPTIA_gain_index(g);
			if (fact < num_lptia_gain_factors - 1) {
				fact++;
				LOG(Log_AD5940, LevelDebug, "Increasing TIA gain to %lu",
						lptia_gain_factors[fact].factor);
				ad5940_set_TIA_gain(a, lptia_gain_factors[fact].gain);
			} else {
				LOG(Log_AD5940, LevelDebug,
						"Would increase TIA gain but reached limit");
			}
		} else if (abs(raw) > HIGH_THRESHOLD) {
			ad5940_tia_gain_t g;
			ad5940_get_TIA_gain(a, &g);
			uint8_t fact = LPTIA_gain_index(g);
			if (fact > 1) {
				fact--;
				LOG(Log_AD5940, LevelDebug, "Decreasing TIA gain to %lu",
						lptia_gain_factors[fact].factor);
				ad5940_set_TIA_gain(a, lptia_gain_factors[fact].gain);
			} else {
				LOG(Log_AD5940, LevelDebug,
						"Would decrease TIA gain but reached limit");
			}
		}
	}

	return AD5940_RES_OK;
}

ad5940_result_t ad5940_enable_FIFO(ad5940_t *a, ad5940_fifosrc_t src) {
	uint32_t val = (1<<11)						// enable FIFO
				| (((uint32_t) src) << 13);		// select source
	ad5940_disable_FIFO(a);
	ad5940_modify_reg(a, AD5940_REG_CMDDATACON, 0x000006C0, 0xFFFFFFE0);
	ad5940_write_reg(a, AD5940_REG_FIFOCON, val);
	ad5940_write_reg(a, AD5940_REG_DATAFIFOTHRES, 100UL << 16);
	return AD5940_RES_OK;
}
ad5940_result_t ad5940_disable_FIFO(ad5940_t *a) {
	ad5940_write_reg(a, AD5940_REG_FIFOCON, 0x0000);
	return AD5940_RES_OK;
}
uint16_t ad5940_get_FIFO_level(ad5940_t *a) {
	uint32_t raw = ad5940_read_reg(a, AD5940_REG_FIFOCNTSTA);
	return ((raw&0x07FF0000) >> 16);
}
static uint16_t readFIFO(ad5940_t *a, uint32_t offset)
{
	uint8_t recv[4];
	HAL_SPI_TransmitReceive(a->spi, (uint8_t*) &offset, recv, 4, 100);
	return recv[3] | (uint16_t) recv[2] << 8;
}
void ad5940_FIFO_read(ad5940_t *a, uint16_t *dest, uint16_t num) {
	if (!num) {
		return;
	}
	if (num < 2) {
		*dest = ad5940_read_reg(a, AD5940_REG_DATAFIFORD) & 0x0000FFFF;
		return;
	}
	cs_low(a);
	// Read FIFO cmd followed by 6 dummy bytes
	uint8_t data[7] = { AD5940_SPICMD_READFIFO, 0, 0, 0, 0, 0, 0 };
	HAL_SPI_Transmit(a->spi, data, 7, 100);
	while (num > 2) {
		*dest++ = readFIFO(a, 0);
		num--;
	}
	*dest++ = readFIFO(a, 0x44444444);
	*dest = readFIFO(a, 0x44444444);
	cs_high(a);
}

ad5940_result_t ad5940_generate_waveform(ad5940_t *a, ad5940_waveinfo_t *w) {
	ad5940_take_mutex(a);
	ad5940_result_t res = AD5940_RES_ERROR;

	// disable waveform before changing any values
	ad5940_clear_bits(a, AD5940_REG_WGCON, 0x06);
	ad5940_clear_bits(a, AD5940_REG_AFECON, (1UL << 14));

	switch (w->type) {
	case AD5940_WAVE_SINE: {
		LOG(Log_AD5940, LevelDebug, "Setting sinewave generation...");
		/*
		 * Calculate frequency word. F_out = F_ACLK(16MHz) * F_CW / 2^30
		 * -> F_CW = F_out * 2^30 / 16MHz.
		 */
		uint32_t f_cw = (uint64_t) w->sine.frequency * (1UL << 30)
				/ 16000000000ULL;
		ad5940_write_reg(a, AD5940_REG_WGFCW, f_cw);
		LOG(Log_AD5940, LevelDebug, "Frequency control word: 0x%08x", f_cw);
		/*
		 * Calculate phase offset. offset_reg = (offset/360)*2^30
		 */
		// first, wrap phase
		while (w->sine.phaseoffset >= 360000) {
			w->sine.phaseoffset -= 360000;
		}
		while (w->sine.phaseoffset < 0) {
			w->sine.phaseoffset -= 360000;
		}
		uint32_t phase_reg = (uint64_t) w->sine.phaseoffset * (1UL << 30)
				/ 360000;
		ad5940_write_reg(a, AD5940_REG_WGPHASE, phase_reg);
		LOG(Log_AD5940, LevelDebug, "Phase offset register: 0x%08x", phase_reg);
		/*
		 * HSDAC has an output range of +/-400mV with the codes 0x200 and 0xE00
		 * representing -400mV and +400mV respectively. Furthermore an attenuator of either 1 or 0.2
		 * as well as an amplifier with an amplification of either 2 or 0.25 follow the HSDAC. The maximum
		 * voltage swing is therefore +/-600mV when the attenuator is set to 1 and the amplifier to 2.
		 */
		uint16_t hsdaccon;
		uint16_t fullscale;
		uint32_t max_voltage = abs(w->sine.offset) + abs(w->sine.amplitude);
		if (max_voltage > 800000) {
			LOG(Log_AD5940, LevelError,
					"Unable to set requested sine wave, required voltage not in available range (%lu > 800)",
					max_voltage);
			break;
		} else if (max_voltage >= 160000) {
			// requires the maximum range, amplifier = 2, attenuator = 1
			hsdaccon = 0x0000;
			// maximum reachable output swing is 800mV in this configuration
			fullscale = 800000;
			LOG(Log_AD5940, LevelDebug,
					"Selected fullscale of 800mV (attenuator = 1, amplifier = 2)");
		} else if (max_voltage >= 100000) {
			// requires attenuator = 0.2, amplifier = 2
			hsdaccon = 0x0001;
			// maximum reachable output swing is 160mV in this configuration
			fullscale = 160000;
			LOG(Log_AD5940, LevelDebug,
					"Selected fullscale of 160mV (attenuator = 0.2, amplifier = 2)");
		} else if (max_voltage >= 20000) {
			// requires attenuator = 1, amplifier = 0.25
			hsdaccon = 0x1000;
			// maximum reachable output swing is 100mV in this configuration
			fullscale = 100000;
			LOG(Log_AD5940, LevelDebug,
					"Selected fullscale of 100mV (attenuator = 1, amplifier = 0.25)");
		} else {
			// can use attenuator = 0.2, amplifier = 0.25
			hsdaccon = 0x1001;
			// maximum reachable output swing is 20mV in this configuration
			fullscale = 20000;
			LOG(Log_AD5940, LevelDebug,
					"Selected fullscale of 20mV (attenuator = 0.2, amplifier = 0.25)");
		}
		ad5940_modify_reg(a, AD5940_REG_HSDACCON, hsdaccon, 0x1001);
		/*
		 * Calculate offset and amplitude now that the fullscale value is known
		 */
		const uint16_t dac_range = 2047;
		int16_t offset = (int32_t) w->sine.offset * dac_range / fullscale;
		int16_t amplitude = (int32_t) w->sine.amplitude * dac_range / fullscale;
		ad5940_modify_reg(a, AD5940_REG_WGOFFSET, offset, 0x0FFF);
		LOG(Log_AD5940, LevelDebug, "Voltage offset register: %d", offset);
		ad5940_modify_reg(a, AD5940_REG_WGAMPLITUDE, amplitude, 0x07FF);
		LOG(Log_AD5940, LevelDebug, "Amplitude register: %d", amplitude);
		// finally, enable the waveform generator and select the sine waveform
		ad5940_modify_reg(a, AD5940_REG_WGCON, 0x0004, 0x0006);
		ad5940_set_bits(a, AD5940_REG_AFECON, (1UL << 14));
		LOG(Log_AD5940, LevelInfo, "Sinewave enabled");
		res = AD5940_RES_OK;
	}
		break;
	default:
		LOG(Log_AD5940, LevelWarn,
				"Ignoring waveform command with unsupported wavetype");
		break;
	}
	ad5940_release_mutex(a);
	return res;
}

ad5940_result_t ad5940_set_excitation_amplifier(ad5940_t *a,
		ad5940_ex_amp_dsw_t dsw, ad5940_ex_amp_psw_t psw,
		ad5940_ex_amp_nsw_t nsw, bool dc_bias) {
	ad5940_take_mutex(a);

	// close selected D, P and N switches
	ad5940_modify_reg(a, AD5940_REG_SWCON, dsw | psw | nsw, 0x0FFF);

	if (dc_bias) {
		// apply voltage set by low power DAC (Vbias and Vzero) to excitation amplifier
		ad5940_set_bits(a, AD5940_REG_AFECON, (1UL << 21));
	} else {
		ad5940_set_bits(a, AD5940_REG_AFECON, (1UL << 21));
	}

	// enable excitation amplifier and HSDAC
	ad5940_set_bits(a, AD5940_REG_AFECON,
			(1UL << 20) | (1UL << 10) | (1UL << 9) | (1UL << 6));
	LOG(Log_AD5940, LevelInfo, "Excitation amplifier connected and enabled");

	ad5940_release_mutex(a);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_disable_excitation_amplifier(ad5940_t *a) {
	ad5940_take_mutex(a);
	// disable excitation amplifier and high speed DAC
	ad5940_clear_bits(a, AD5940_REG_AFECON,
			(1UL << 20) | (1UL << 10) | (1UL << 9) | (1UL << 6));
	// open all D switches, close PL and NL for feedback stability once the amplifier is turned on again
	ad5940_clear_bits(a, AD5940_REG_SWCON, 0x0FFF);
	LOG(Log_AD5940, LevelInfo,
			"High speed excitation amplifier disconnected and disabled");
	ad5940_release_mutex(a);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_set_HSTIA(ad5940_t *a, ad5940_hsrtia_t rtia,
		uint8_t ctia, ad5940_HSTIA_VBIAS_t bias, bool diode, ad5940_hstsw_t sw) {
	ad5940_take_mutex(a);
	ad5940_result_t res = AD5940_RES_OK;
	// set bias voltage at positive input of TIA
	ad5940_write_reg(a, AD5940_REG_HSTIACON, (int) bias);
	ad5940_set_bits(a, AD5940_REG_LPDACSW0, 0x01);
	uint32_t regval = 0;
	// set selected capacitor
	if (ctia >= 16) {
		regval |= (0x08UL << 5);
		ctia -= 16;
	}
	if (ctia >= 8) {
		regval |= (0x04UL << 5);
		ctia -= 8;
	}
	if (ctia >= 4) {
		regval |= (0x02UL << 5);
		ctia -= 4;
	}
	if (ctia >= 2) {
		regval |= (0x10UL << 5);
		ctia -= 2;
	}
	// There are two 2pF caps available
	if (ctia >= 2) {
		regval |= (0x10UL << 5);
	}
	// set selected resistor
	regval |= ((uint16_t) rtia) & 0x0F;
	// enable diode
	if (diode) {
		regval |= 0x10;
	}
	ad5940_write_reg(a, AD5940_REG_HSRTIACON, regval);
	// close switch T9 to connect RTIA
	ad5940_set_bits(a, AD5940_REG_SWCON, (1UL<<17));
	// enable HSTIA
	ad5940_set_bits(a, AD5940_REG_AFECON, (1UL << 11));
	// connect to requested pin
	ad5940_modify_reg(a, AD5940_REG_SWCON, sw, 0xF000);
	ad5940_release_mutex(a);
	return res;
}

ad5940_result_t ad5940_disable_HSTIA(ad5940_t *a) {
	ad5940_take_mutex(a);
	ad5940_result_t res = AD5940_RES_OK;
	// disable HSTIA
	ad5940_clear_bits(a, AD5940_REG_AFECON, (1UL << 11));
	// open all T switches
	ad5940_clear_bits(a, AD5940_REG_SWCON, (3UL << 17) | 0xF000);
	ad5940_release_mutex(a);
	return res;
}

static uint8_t HSTIA_gain_index(ad5940_hsrtia_t g) {
	for (uint8_t i = 0; i < num_hstia_gain_factors; i++) {
		if (hstia_gain_factors[i].gain == g) {
			return i;
		}
	}
	return 0;
}

uint32_t ad5940_HSTIA_gain_to_value(ad5940_hsrtia_t g) {
	return hstia_gain_factors[HSTIA_gain_index(g)].factor;
}

ad5940_hsrtia_t ad5940_value_to_HSTIA_gain(uint32_t value) {
	for (uint8_t i = 0; i < num_hstia_gain_factors; i++) {
		if (hstia_gain_factors[i].factor == value) {
			return hstia_gain_factors[i].gain;
		}
	}
	LOG(Log_AD5940, LevelError, "HSTIA gain with factor %lu not available", value);
	return 0;
}

ad5940_result_t ad5940_set_dft(ad5940_t *a, ad5940_dftconfig_t *dft) {
	ad5940_take_mutex(a);
	ad5940_result_t res = AD5940_RES_ERROR;
	ad5940_set_bits(a, AD5940_REG_ADCFILTERCON, (1UL << 18));
	ad5940_clear_bits(a, AD5940_REG_AFECON, (1UL << 15));
	// disable interrupt
	ad5940_clear_bits(a, AD5940_REG_INTCSEL0, 0x02);
	if (dft->source != AD5940_DFTSRC_DISABLED) {
		if (dft->hanning) {
			ad5940_set_bits(a, AD5940_REG_DFTCON, 0x0001);
		} else {
			ad5940_clear_bits(a, AD5940_REG_DFTCON, 0x0001);
		}
		// set DFT source
		switch (dft->source) {
		case AD5940_DFTSRC_AVG:
			ad5940_set_bits(a, AD5940_REG_ADCFILTERCON, (1UL << 7));
			break;
		case AD5940_DFTSRC_RAW:
			ad5940_clear_bits(a, AD5940_REG_ADCFILTERCON, (1UL << 7));
			ad5940_modify_reg(a, AD5940_REG_DFTCON, (0x02 << 20), (0x03 << 20));
			break;
		case AD5940_DFTSRC_SINC2:
			ad5940_clear_bits(a, AD5940_REG_ADCFILTERCON, (1UL << 7));
			ad5940_modify_reg(a, AD5940_REG_DFTCON, (0x00 << 20), (0x03 << 20));
			break;
		case AD5940_DFTSRC_SINC3:
			ad5940_clear_bits(a, AD5940_REG_ADCFILTERCON, (1UL << 7));
			ad5940_modify_reg(a, AD5940_REG_DFTCON, (0x01 << 20), (0x03 << 20));
			break;
		default:
			break;
		}
		// configure points
		uint16_t dftnumval = 29 - __builtin_clz(dft->points);
		if (dftnumval > 0x0C) {
			LOG(Log_AD5940, LevelError,
					"Invalid number of DFT points, %d should be between 4 and 16384");
		} else {
			LOG(Log_AD5940, LevelDebug, "DFTNUM: %u, numval: 0x%02x", dft->points, dftnumval);
			ad5940_modify_reg(a, AD5940_REG_DFTCON, dftnumval << 4, (0x0F << 4));
			res = AD5940_RES_OK;
		}
		uint32_t status = ad5940_read_reg(a, AD5940_REG_DFTCON);
		LOG(Log_AD5940, LevelDebug, "DFTCON status: 0x%08x", status);
		// enable interrupt and clear possible old flag
		ad5940_set_bits(a, AD5940_REG_INTCSEL0, 0x02);
		ad5940_write_reg(a, AD5940_REG_INTCCLR, 0x02);
		// enable dft
		ad5940_clear_bits(a, AD5940_REG_ADCFILTERCON, (1UL << 18));
		ad5940_set_bits(a, AD5940_REG_AFECON, (1UL << 15));
	}
	ad5940_release_mutex(a);
	return res;
}

ad5940_result_t ad5940_get_dft_result(ad5940_t *a, uint8_t avg, ad5940_dftresult_t *data) {
	ad5940_take_mutex(a);
	data->mag = 0.0f;
	data->phase = 0.0f;
	for (uint8_t i = 0; i < avg; i++) {
		// clear possible previous interrupt
		ad5940_write_reg(a, AD5940_REG_INTCCLR, 0x02);
		// wait for DFT to finish
		uint32_t start = HAL_GetTick();
		while (!(ad5940_read_reg(a, AD5940_REG_INTCFLAG0) & 0x02)) {
			portYIELD();
			if (HAL_GetTick() - start > 30) {
				LOG(Log_AD5940, LevelWarn, "Timed out waiting for DFT result");
				ad5940_release_mutex(a);
				return AD5940_RES_ERROR;
			}
		}

		int32_t real = util_sign_extend_32(ad5940_read_reg(a, AD5940_REG_DFTREAL), 18);
		int32_t imag = util_sign_extend_32(ad5940_read_reg(a, AD5940_REG_DFTIMAG), 18);
		LOG(Log_AD5940, LevelDebug, "DFT raw R: %ld, I: %ld", real, imag);
		float f_real = (float) real / (1UL << 17);
		float f_imag = (float) imag / (1UL << 17);
		data->mag += sqrt(f_real * f_real + f_imag * f_imag);
		data->phase += atan2(-f_imag, f_real);
	}
	data->mag /= avg;
	data->phase /= avg;
	ad5940_release_mutex(a);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_setup_four_wire(ad5940_t *a, uint32_t frequency,
		uint32_t nS_min, uint32_t nS_max, ad5940_ex_amp_dsw_t amp,
		ad5940_hstsw_t hstsw, uint16_t rseries) {
	ad5940_take_mutex(a);
	LOG(Log_AD5940, LevelInfo, "Configuring for four wire measurement...");
#define EX_AMP_MAX_AMPLITUDE		800
	uint16_t amplitude = EX_AMP_MAX_AMPLITUDE;
	// max. expected current in nA
	uint32_t max_current = 1000000UL * EX_AMP_MAX_AMPLITUDE
			/ (rseries + 1000000000UL / nS_max);
	if (EX_AMP_MAX_AMPLITUDE / (rseries + 1000000000UL / nS_max) >= 3) {
		amplitude = 3 * (rseries + 1000000000UL / nS_max);
		LOG(Log_AD5940, LevelDebug,
				"Maximum possible current is too high, reduced amplitude of waveform to %umV",
				amplitude);
		max_current = 3000000;
	}

	ad5940_waveinfo_t wave;
	wave.type = AD5940_WAVE_SINE;
	wave.sine.amplitude = amplitude;
	wave.sine.frequency = frequency;
	wave.sine.offset = 0;
	wave.sine.phaseoffset = 0;
	// start waveform generation
	ad5940_generate_waveform(a, &wave);

	LOG(Log_AD5940, LevelDebug, "Maximum possible current: %lunA", max_current);
	// calculate best fitting RTIA
	// maximum output should be <=900mV
	uint32_t max_gain = 900 * 1000000 / max_current;
	LOG(Log_AD5940, LevelDebug, "Maximum allowed gain: %lu", max_gain);
	uint8_t i;
	for (i = 0; i < num_hstia_gain_factors; i++) {
		if (hstia_gain_factors[i].factor > max_gain) {
			break;
		}
	}
	// went one gain factor too far
	i--;
	uint32_t gain_factor = hstia_gain_factors[i].factor;
	a->impedance.rtia = hstia_gain_factors[i].gain;
	LOG(Log_AD5940, LevelDebug, "Selected gain: %lu", gain_factor);
	ad5940_set_HSTIA(a, a->impedance.rtia, 30, AD5940_HSTIA_VBIAS_1V1, false, hstsw);
	// calculate maximum output voltage of TIA in mV
	// always use lowest gain (higher gains did not result in any improvements)
	a->impedance.gain_current = AD5940_PGA_GAIN_1;
//	// see datasheet page 7 "ADC input voltage ranges" for values
//	uint32_t max_TIA_voltage = (uint64_t) (max_current) * gain_factor / 1000000UL;
//	if (max_TIA_voltage <= 133) {
//		a->impedance.gain_current = AD5940_PGA_GAIN_9;
//	} else if (max_TIA_voltage <= 300) {
//		a->impedance.gain_current = AD5940_PGA_GAIN_4;
//	} else if (max_TIA_voltage <= 600) {
//		a->impedance.gain_current = AD5940_PGA_GAIN_2;
//	} else if (max_TIA_voltage <= 900) {
//		a->impedance.gain_current = AD5940_PGA_GAIN_1_5;
//	} else {
//		a->impedance.gain_current = AD5940_PGA_GAIN_1;
//	}
	LOG(Log_AD5940, LevelDebug, "Selected current PGA gain: %f",
			(float ) ad5940_PGA_gain_to_value10(a->impedance.gain_current)
					/ 10);

	// maximum possible voltage occurs with minimal conductivity
	uint32_t max_voltage = max_current * 1000ULL / nS_min; // in mV
	if (max_voltage > amplitude) {
		max_voltage = amplitude;
	}
	LOG(Log_AD5940, LevelDebug, "Maximum possible voltage: %lumV", max_voltage);
	// see datasheet page 7 "ADC input voltage ranges" for values
	if (max_voltage <= 133) {
		a->impedance.gain_voltage = AD5940_PGA_GAIN_9;
	} else if (max_voltage <= 300) {
		a->impedance.gain_voltage = AD5940_PGA_GAIN_4;
	} else if (max_voltage <= 600) {
		a->impedance.gain_voltage = AD5940_PGA_GAIN_2;
	} else if (max_voltage <= 900) {
		a->impedance.gain_voltage = AD5940_PGA_GAIN_1_5;
	} else {
		a->impedance.gain_voltage = AD5940_PGA_GAIN_1;
	}
	LOG(Log_AD5940, LevelDebug, "Selected voltage PGA gain: %f",
			(float ) ad5940_PGA_gain_to_value10(a->impedance.gain_voltage)
					/ 10);

	// connect and enable excitation amplifier to the selected output
	ad5940_set_excitation_amplifier(a, amp, AD5940_EXAMP_PSW_INT_FEEDBACK,
			AD5940_EXAMP_NSW_INT_FEEDBACK, false);

	uint32_t status = ad5940_read_reg(a, AD5940_REG_DSWSTA);
	LOG(Log_AD5940, LevelDebug, "Switch D status: 0x%08x", status);
	status = ad5940_read_reg(a, AD5940_REG_PSWSTA);
	LOG(Log_AD5940, LevelDebug, "Switch P status: 0x%08x", status);
	status = ad5940_read_reg(a, AD5940_REG_NSWSTA);
	LOG(Log_AD5940, LevelDebug, "Switch N status: 0x%08x", status);
	status = ad5940_read_reg(a, AD5940_REG_TSWSTA);
	LOG(Log_AD5940, LevelDebug, "Switch T status: 0x%08x", status);
	status = ad5940_read_reg(a, AD5940_REG_AFECON);
	LOG(Log_AD5940, LevelDebug, "AFECON status: 0x%08x", status);
	status = ad5940_read_reg(a, AD5940_REG_WGCON);
	LOG(Log_AD5940, LevelDebug, "WGCON status: 0x%08x", status);
	ad5940_release_mutex(a);
	return AD5940_RES_OK;
}

ad5940_result_t ad5940_measure_four_wire(ad5940_t *a, ad5940_adc_muxp_t p_sense,
		ad5940_adc_muxn_t n_sense, uint8_t avg, ad5940_dftresult_t *data) {
	ad5940_take_mutex(a);
	ad5940_result_t res = AD5940_RES_OK;
//	res |= ad5940_set_ADC_mux(a, p_sense, n_sense);
//	ad5940_set_PGA_gain(a, a->impedance.gain_voltage);
//	res |= ad5940_ADC_start(a);
//	ad5940_dftresult_t dft_voltage;
//	res |= ad5940_get_dft_result(a, 1, &dft_voltage);
//	res |= ad5940_get_dft_result(a, avg, &dft_voltage);
//	res |= ad5940_ADC_stop(a);
	res |= ad5940_set_ADC_mux(a, AD5940_ADC_MUXP_HSTIAP, AD5940_ADC_MUXN_HSTIAN);
	ad5940_set_PGA_gain(a, a->impedance.gain_current);
	res |= ad5940_ADC_start(a);
	ad5940_dftresult_t dft_current;
	res |= ad5940_get_dft_result(a, 1, &dft_current);
	res |= ad5940_get_dft_result(a, avg, &dft_current);
	res |= ad5940_ADC_stop(a);
//	dft_voltage.mag /= (float) ad5940_PGA_gain_to_value10(
//			a->impedance.gain_voltage) / 10;
	dft_current.mag /= (float) ad5940_PGA_gain_to_value10(
			a->impedance.gain_current) / 10
			* ad5940_HSTIA_gain_to_value(a->impedance.rtia);
	data->mag = /*dft_voltage.mag*/ 1.0f / dft_current.mag;
	data->phase = /*dft_voltage.phase*/ 0 - dft_current.phase;

	return res;
}

ad5940_result_t ad5940_disable_HS_loop(ad5940_t *a) {
	ad5940_result_t res = AD5940_RES_OK;
	ad5940_take_mutex(a);
	ad5940_clear_bits(a, AD5940_REG_SWCON, (1UL<<16)); // control switches as group
	res |= ad5940_disable_excitation_amplifier(a);
	res |= ad5940_disable_HSTIA(a);
	// stop waveform generation and DFT
	ad5940_clear_bits(a, AD5940_REG_AFECON, (1UL << 14) | (1UL << 15));
	ad5940_release_mutex(a);
	return res;
}

ad5940_result_t ad5940_gpio_configure(ad5940_t *a, uint8_t gpio, ad5940_gpio_config_t config) {
	ad5940_take_mutex(a);
	// general purpose I/O on all GPIOs
	ad5940_write_reg(a, AD5940_REG_GP0CON, 0x0023);
	switch(config) {
	case AD5940_GPIO_DISABLED:
		ad5940_clear_bits(a, AD5940_REG_GP0OEN, gpio);
		ad5940_clear_bits(a, AD5940_REG_GP0PE, gpio);
		ad5940_clear_bits(a, AD5940_REG_GP0IEN, gpio);
		break;
	case AD5940_GPIO_INPUT:
		ad5940_clear_bits(a, AD5940_REG_GP0OEN, gpio);
		ad5940_clear_bits(a, AD5940_REG_GP0PE, gpio);
		ad5940_set_bits(a, AD5940_REG_GP0IEN, gpio);
		break;
	case AD5940_GPIO_OUTPUT:
		ad5940_set_bits(a, AD5940_REG_GP0OEN, gpio);
		ad5940_clear_bits(a, AD5940_REG_GP0PE, gpio);
		ad5940_clear_bits(a, AD5940_REG_GP0IEN, gpio);
		break;
	}
	ad5940_release_mutex(a);
	return AD5940_RES_OK;
}
ad5940_result_t ad5940_gpio_set(ad5940_t *a, uint8_t gpio) {
	ad5940_take_mutex(a);
	ad5940_write_reg(a, AD5940_REG_GP0SET, gpio);
	ad5940_release_mutex(a);
	if (gpio & AD5940_GPIO2) {
		// GPIO 2 can not be used as general purpose. Switch to POR (always high)
		ad5940_clear_bits(a, AD5940_REG_GP0CON, 0x0030);
	}
	return AD5940_RES_OK;
}
ad5940_result_t ad5940_gpio_clear(ad5940_t *a, uint8_t gpio) {
	ad5940_take_mutex(a);
	ad5940_write_reg(a, AD5940_REG_GP0CLR, gpio);
	if (gpio & AD5940_GPIO2) {
		// GPIO 2 can not be used as general purpose. Switch to sync output (always low as synchronization is not used)
		ad5940_modify_reg(a, AD5940_REG_GP0CON, 0x0020, 0x0030);
	}
	ad5940_release_mutex(a);
	return AD5940_RES_OK;
}
uint8_t ad5940_gpio_get(ad5940_t *a, uint8_t gpio) {
	ad5940_take_mutex(a);
	uint8_t in = ad5940_read_reg(a, AD5940_REG_GP0IN);
	ad5940_release_mutex(a);
	return in & gpio;
}

