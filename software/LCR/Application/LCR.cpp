#include "LCR.hpp"

#include "Waveform.hpp"
#include "Algorithm.hpp"
#include "pga280.h"
#include "FreeRTOS.h"
#include "task.h"
#include "log.h"
#include <algorithm>
#include <cmath>
#include <cstring>

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
constexpr uint8_t MaxGainSetting = sizeof(GainSettings)/sizeof(GainSettings[0]) - 1;

static void Print(int16_t *data, uint16_t n) {
	for (uint16_t i = 0; i < n; i++) {
		LOG(Log_Wave, LevelInfo, "%d", data[i]);
		log_flush();
	}
}

bool LCR::Init() {
	pga.CSgpio = GPIOB;
	pga.CSpin = GPIO_PIN_12;
	pga.spi = &hspi2;
	return pga280_init(&pga) == PGA280_RES_OK;
}

static bool FindOptimalGain(uint8_t *gain, uint32_t freq) {
	// calculate maximum gain
	constexpr uint32_t PGAGBW = 6000000;
	const float MaxGain = PGAGBW / freq;
	uint8_t IndexMaxGain = MaxGainSetting;
	while (GainSettings[IndexMaxGain].gain > MaxGain) {
		IndexMaxGain--;
	}
	LOG(Log_LCR, LevelDebug,
			"Maximum theoretical PGA gain for this frequency is %f",
			MaxGain);
	LOG(Log_LCR, LevelDebug, "Actual PGA gain limited to %f",
			GainSettings[IndexMaxGain].gain);
	if(*gain > IndexMaxGain) {
		*gain = IndexMaxGain;
	}
	LOG(Log_LCR, LevelDebug, "Finding optimal gain");
	auto data = Waveform::Get();
	const uint32_t samplingDelay = data.nvalues * 1000 / data.samplerate + 30;
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
		constexpr uint16_t ADCTop = 2047 - SafetyMargin;
		constexpr uint16_t ADCBot = -2048 + SafetyMargin;
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
			if (*gain < IndexMaxGain) {
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
	LOG(Log_LCR, LevelDebug, "Optimal gain: %f", GainSettings[*gain].gain);
	return true;
}

static int16_t* SampleAndAverage(uint16_t waveforms, uint32_t maxTime) {
	if(waveforms > 16) {
		waveforms = 16;
	}
	auto data = Waveform::Get();
	int16_t *average = new int16_t[data.nvalues];
	if(!average) {
		LOG(Log_LCR, LevelError, "Couldn't allocate memory for samples");
		return nullptr;
	}
	memset(average, 0, data.nvalues * sizeof(*average));
	const uint32_t samplingDelay = data.nvalues * 1000 / data.samplerate + 2;
	if (waveforms * samplingDelay > maxTime) {
		waveforms = maxTime / samplingDelay;
	}
	std::copy(data.values, data.values + data.nvalues, average);
	for (uint16_t i = 0; i < waveforms; i++) {
		vTaskDelay(samplingDelay);
		for(uint16_t j=0;j<data.nvalues;j++) {
			average[j] += data.values[j];
		}
	}
	for(uint16_t j=0;j<data.nvalues;j++) {
		average[j] /= waveforms;
	}
	return average;
}

bool LCR::Measure(uint32_t frequency, double *real, double *imag, double *phase) {
//	Waveform::Setup(frequency, 2048);
//	Waveform::Enable();
	LOG(Log_LCR, LevelInfo, "Taking voltage measurement");
	pga280_select_input(&pga, PGA280_INPUT2_DIFF);
	if (!FindOptimalGain(&VoltageGainSetting, frequency)) {
		LOG(Log_LCR, LevelWarn, "Measurement failed, voltage input saturated");
		return false;
	}
	// save voltage data
	auto data = Waveform::Get();
	LOG(Log_LCR, LevelInfo, "Saving voltage measurement");
	int16_t *voltage = SampleAndAverage(16, 500);
	if(!voltage) {
		LOG(Log_LCR, LevelError, "Couldn't allocate memory for voltage samples");
		return false;
	}
	Print(voltage, data.nvalues);
	LOG(Log_LCR, LevelInfo, "Taking Current measurement");
	pga280_select_input(&pga, PGA280_INPUT1_DIFF);
	HAL_GPIO_WritePin(TRIGGER_GPIO_Port, TRIGGER_Pin, GPIO_PIN_SET);
	if (!FindOptimalGain(&CurrentGainSetting, frequency)) {
		LOG(Log_LCR, LevelWarn, "Measurement failed, current input saturated");
		return false;
	}
	HAL_GPIO_WritePin(TRIGGER_GPIO_Port, TRIGGER_Pin, GPIO_PIN_RESET);
	// save current data
	LOG(Log_LCR, LevelInfo, "Saving current measurement");
	int16_t *current = SampleAndAverage(16, 500);
	if(!current) {
		LOG(Log_LCR, LevelError, "Couldn't allocate memory for current samples");
		return false;
	}
	Print(current, data.nvalues);

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
	*phase = Algorithm::PhaseDiff(voltage, current, data.nvalues);
	LOG(Log_LCR, LevelDebug, "Phase = %f", *phase * 180 / 3.141592f);

	double impedance = voltageRMS / currentRMS;

	*real = impedance * cos(*phase);
	*imag = impedance * sin(*phase);
	LOG(Log_LCR, LevelDebug, "Impedance: %fOhm/%fjOhm", *real, *imag);

	delete voltage;
	delete current;
	return true;
}

bool LCR::CalibratePhase() {
	uint32_t freq = 1000;
	for (uint16_t i = 2048; i > 4; i >>= 1) {
		Waveform::Setup(freq, i);
		Waveform::Enable();
		pga280_select_input(&pga, PGA280_INPUT1_DIFF);
		vTaskDelay(3000);
		FindOptimalGain(&CurrentGainSetting, freq);
		Waveform::Disable();
		auto data = Waveform::Get();
		Algorithm::RemoveDC(data.values, data.nvalues);
		double phase = Algorithm::Phase(data.values, data.nvalues);
		LOG(Log_LCR, LevelInfo, "Phase offset: %f", phase);
	}
}
