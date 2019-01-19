#include "LCR.hpp"

#include "Waveform.hpp"
#include "Algorithm.hpp"
#include "pga280.h"
#include "FreeRTOS.h"
#include "task.h"
#include "log.h"
#include <algorithm>
#include <cmath>

static pga280_t pga;
extern SPI_HandleTypeDef hspi2;

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

static uint8_t VoltageGainSetting = 3;
static uint8_t CurrentGainSetting = 3;
constexpr uint8_t MaxGainSetting = sizeof(GainSettings)/sizeof(GainSettings[0]);

bool LCR::Init() {
	pga.CSgpio = GPIOB;
	pga.CSpin = GPIO_PIN_12;
	pga.spi = &hspi2;
	return pga280_init(&pga) == PGA280_RES_OK;
}

static bool FindOptimalGain(uint8_t *gain) {
	LOG(Log_LCR, LevelDebug, "Finding optimal gain");
	auto data = Waveform::Get();
	const uint32_t samplingDelay = data.nvalues * 1000 / data.samplerate + 1;
	bool adjusted = false;
	do {
		pga280_set_gain(&pga, GainSettings[*gain].setting, false);
		vTaskDelay(samplingDelay);
		// check range
		int16_t min = INT16_MAX;
		int16_t max = INT16_MIN;
		for (uint16_t i = 0; i < data.nvalues; i++) {
			if (data.values[i] < min) {
				min = data.values[i];
			} else if (data.values[i] > max) {
				max = data.values[i];
			}
		}
		constexpr uint16_t SafetyMargin = 100;
		constexpr uint16_t ADCTop = 4095 - SafetyMargin;
		constexpr uint16_t ADCBot = 0 + SafetyMargin;
		int16_t headroom = ADCTop - max;
		int16_t footroom = min - ADCBot;
		LOG(Log_LCR, LevelDebug, "Headroom %d, footroom %d", headroom,
				footroom);
		if (headroom < 0 || footroom < 0) {
			if (*gain > 0) {
				LOG(Log_LCR, LevelDebug, "Decreasing gain");
				(*gain)--;
			} else {
				// reached lowest gain setting, still overloaded
				LOG(Log_LCR, LevelDebug, "Would decrease gain but reached limit");
				return false;
			}
		} else if (headroom > Waveform::ADCMax / 4
				&& footroom > Waveform::ADCMax / 4) {
			if (*gain < MaxGainSetting - 1) {
				LOG(Log_LCR, LevelDebug, "Increasing gain");
				(*gain)++;
			} else {
				// could use more amplification but its not available
				adjusted = true;
				LOG(Log_LCR, LevelDebug,
						"Would increase gain but reached limit");
			}
		} else {
			adjusted = true;
			LOG(Log_LCR, LevelDebug, "Optimal gain found");
		}
	} while(!adjusted);
	return true;
}

bool LCR::Measure(uint32_t frequency, double *real, double *imag) {
//	Waveform::Setup(frequency, 2048);
//	Waveform::Enable();
	LOG(Log_LCR, LevelInfo, "Taking voltage measurement");
	pga280_select_input(&pga, PGA280_INPUT2_DIFF);
	if (FindOptimalGain(&VoltageGainSetting)) {
		LOG(Log_LCR, LevelDebug, "Optimal gain for voltage measurement: %f",
				GainSettings[VoltageGainSetting].gain);
	} else {
		LOG(Log_LCR, LevelWarn, "Measurement failed, voltage input saturated");
		return false;
	}
	// save voltage data
	auto data = Waveform::Get();
	LOG(Log_LCR, LevelInfo, "Saving voltage measurement");
	int16_t *voltage = new int16_t[data.nvalues];
	if(!voltage) {
		LOG(Log_LCR, LevelError, "Couldn't allocate memory for voltage samples");
		return false;
	}
	std::copy(data.values, data.values + data.nvalues, voltage);
	LOG(Log_LCR, LevelInfo, "Taking Current measurement");
	pga280_select_input(&pga, PGA280_INPUT1_DIFF);
	if (FindOptimalGain(&CurrentGainSetting)) {
		LOG(Log_LCR, LevelDebug, "Optimal gain for current measurement: %f",
				GainSettings[CurrentGainSetting].gain);
	} else {
		LOG(Log_LCR, LevelWarn, "Measurement failed, current input saturated");
		return false;
	}
	// save current data
	LOG(Log_LCR, LevelInfo, "Saving current measurement");
	int16_t *current = new int16_t[data.nvalues];
	if(!current) {
		LOG(Log_LCR, LevelError, "Couldn't allocate memory for current samples");
		return false;
	}
	std::copy(data.values, data.values + data.nvalues, current);

//	Waveform::Disable();

	// evaluate measurements
	Algorithm::RemoveDC(voltage, data.nvalues);
	Algorithm::RemoveDC(current, data.nvalues);
	double voltageRMS = Waveform::Reference
			* Algorithm::RMS(voltage, data.nvalues) / (Waveform::ADCMax / 2);
	voltageRMS /= GainSettings[VoltageGainSetting].gain;
	LOG(Log_LCR, LevelDebug, "RMS voltage = %f", voltageRMS);
	double currentRMS = Waveform::Reference
			* Algorithm::RMS(current, data.nvalues) / (Waveform::ADCMax / 2);
	currentRMS /= GainSettings[CurrentGainSetting].gain;
	// account for current shunt
	currentRMS /= 909;
	LOG(Log_LCR, LevelDebug, "RMS current = %f", currentRMS);
	double phase = Algorithm::PhaseDiff(voltage, current, data.nvalues);
	LOG(Log_LCR, LevelDebug, "Phase = %f°", phase * 180 / 3.141592f);

	double realCurrentRMS = currentRMS * cos(phase);
	double imagCurrentRMS = currentRMS * sin(phase);

	*real = voltageRMS / realCurrentRMS;
	*imag = voltageRMS / imagCurrentRMS;
	LOG(Log_LCR, LevelDebug, "Impedance: %fOhm/%fjOhm", *real, *imag);

	delete voltage;
	delete current;
	return true;
}
