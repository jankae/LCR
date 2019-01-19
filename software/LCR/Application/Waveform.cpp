#include <Waveform.hpp>
#include <cmath>
#include "stm.h"
#include "log.h"

/*
 * Clock-setup:
 * TIM2 clocked by 72MHz internal clock, reload is adjusted depending on frequency
 * -> Triggers ADC on update event
 * -> Triggers TIM3 on update event
 * TIM3 always has a prescaler of 0 and a reload value of 3
 * -> Triggers DAC on update event
 *
 * -> DAC rate is always 1/4 of the ADC rate
 */

constexpr uint16_t DACMaxValue = 4095;
constexpr uint16_t MemoryLengthDAC = 500;
constexpr uint32_t TimerClk = 72000000;
constexpr uint32_t DACMaxRate = 1000000;
static int16_t DACMemory[MemoryLengthDAC];
static int16_t ADCMemory[MemoryLengthDAC * 4];

extern DAC_HandleTypeDef hdac;
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2, htim3;

static uint32_t DACValues;

void Waveform::Setup(uint32_t freq, uint16_t peakval) {
	DACValues = DACMaxRate / freq;
	if(DACValues > MemoryLengthDAC) {
		DACValues = MemoryLengthDAC;
	}
	uint32_t DACRate = freq * DACValues;
	uint32_t reload = TimerClk / DACRate;
	TIM2->ARR = (reload/4) - 1;

	// calculate actual DAC update rate

	// calculate sine wave
	for (uint16_t i = 0; i < DACValues; i++) {
		DACMemory[i] = DACMaxValue / 2
				+ sin(2 * M_PI * (float) i / DACValues) * peakval;
		if (DACMemory[i] < 0) {
			DACMemory[i] = 0;
		} else if (DACMemory[i] > DACMaxValue) {
			DACMemory[i] = DACMaxValue;
		}
	}
}

void Waveform::Enable() {
	HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*) DACMemory, DACValues,
			DAC_ALIGN_12B_R);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) ADCMemory, DACValues * 4);
	HAL_TIM_Base_Start(&htim3);
	HAL_TIM_Base_Start(&htim2);
}

void Waveform::Disable() {
	HAL_ADC_Stop_DMA(&hadc1);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
	HAL_TIM_Base_Stop(&htim3);
	HAL_TIM_Base_Stop(&htim2);
}

void Waveform::PrintADC() {
	for(uint16_t i=0;i<DACValues * 4;i++) {
		LOG(Log_Wave, LevelInfo, "%d", ADCMemory[i]);
		log_flush();
	}
}

Waveform::Data Waveform::Get() {
	Data ret;
	ret.values = ADCMemory;
	ret.nvalues = DACValues * 4;
	ret.samplerate = TimerClk / (TIM2->ARR + 1);
	ret.tone = ret.samplerate / DACValues;
	return ret;
}
