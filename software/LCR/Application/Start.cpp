#include "Start.h"
#include "log.h"
#include "Algorithm.hpp"
#include <cstring>
#include "LCR.hpp"
#include "Waveform.hpp"
#include <cmath>

#include "pga280.h"
using PGAGain = struct {
	pga280_gain_t setting;
	float gain;
};

constexpr PGAGain GainSettings[] = {
		{PGA280_GAIN_1_8, 0.125f},
		{PGA280_GAIN_1_4, 0.25f},
		{PGA280_GAIN_1_2, 0.5f},
		{PGA280_GAIN_1, 1.0f},
		{PGA280_GAIN_2, 2.0f},
		{PGA280_GAIN_4, 4.0f},
		{PGA280_GAIN_8, 8.0f},
		{PGA280_GAIN_16, 16.0f},
		{PGA280_GAIN_32, 32.0f},
		{PGA280_GAIN_64, 64.0f},
		{PGA280_GAIN_128, 128.0f},
};
constexpr uint8_t MaxGainSetting = sizeof(GainSettings)/sizeof(GainSettings[0]) - 1;

pga280_t pga;
extern SPI_HandleTypeDef hspi2;

void Start() {
	log_init();
	LOG(Log_App, LevelInfo, "Start");

//	pga.CSgpio = GPIOB;
//	pga.CSpin = GPIO_PIN_12;
//	pga.spi = &hspi2;
//	pga280_init(&pga);
//
//	pga280_select_input(&pga, PGA280_INPUT1_DIFF);
//	for(uint8_t i=0;i<=MaxGainSetting;i++) {
//		pga280_set_gain(&pga, GainSettings[i].setting, false);
//		LOG(Log_App, LevelInfo, "Set gain: %f", GainSettings[i].gain);
//		log_flush();
//		__BKPT();
//	}
//	while(1);

	LCR::Init();
//	LCR::CalibratePhase();
//	while(1);
	double real, imag, phase;
	constexpr uint32_t freq = 10000;
	Waveform::Setup(freq, 2048);
	Waveform::Enable();

	pga.CSgpio = GPIOB;
	pga.CSpin = GPIO_PIN_12;
	pga.spi = &hspi2;
	pga280_init(&pga);

	// short, voltage measurement
	pga280_select_input(&pga, PGA280_INPUT2_DIFF);
	pga280_set_gain(&pga, PGA280_GAIN_128, false);
	while(1) {
		vTaskDelay(1000);
	}
	while (1) {
		LCR::Measure(freq, &real, &imag, &phase);
		if(phase < -M_PI_4/2) {
			// got a cap
			double cap = 1.0 / (2 * M_PI * -imag * freq);
			double Q = -imag / real;
			LOG(Log_App, LevelInfo, "C=%fnF, Q=%f, ESR=%fOhm", cap * 1000000000, Q, real);
		} else if(phase > M_PI_4/2) {
			// got an inductor
			double ind = imag / (2 * M_PI * freq);
			double Q = imag / real;
			LOG(Log_App, LevelInfo, "L=%fuH, Q=%f, ESR=%fOhm", ind * 1000000, Q, real);
		} else {
			LOG(Log_App, LevelInfo, "R=%fOhm", real);
		}
		vTaskDelay(5000);
	}
}
