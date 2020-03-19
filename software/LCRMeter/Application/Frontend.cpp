#include "Frontend.hpp"
#include "AD5940/ad5940.h"
#include "main.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <math.h>
#include <util.h>
#include "Persistence.hpp"

static ad5940_t ad;
extern SPI_HandleTypeDef hspi3;

#define Log_Frontend (LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

static constexpr uint32_t referenceVoltage = 1100000;
static Frontend::Callback callback;
static void *cb_ctx;

static constexpr uint32_t stack_size_words = 512;
static uint32_t task_stack[stack_size_words];
static TaskHandle_t taskHandle;
static StaticTask_t task;
static StaticQueue_t msgQueue;
static QueueHandle_t queueHandle;

enum class MessageType : uint8_t {
	MeasurementConfig,
	StopMeasurement,
	RunCalibration,
};

using Message = struct {
	MessageType type;
	Frontend::Settings settings;
};

using Calibration = struct {
	float MagCal;
	float PhaseCal;
};

static constexpr uint32_t calibration_frequencies[] = {
		100, 120, 1000, 10000, 100000, 200000
};

static constexpr uint8_t num_rtia_steps = AD5940_HSRTIA_OPEN;

static Calibration calibration_points[num_rtia_steps][ARRAY_SIZE(calibration_frequencies)];

static float interpolate(float value, float scaleFromLow, float scaleFromHigh,
        float scaleToLow, float scaleToHigh) {
	float result;
	value -= scaleFromLow;
	float rangeFrom = scaleFromHigh - scaleFromLow;
	float rangeTo = scaleToHigh - scaleToLow;
	result = (value * rangeTo) / rangeFrom;
	result += scaleToLow;
	return result;
}

static Calibration GetCalibration(ad5940_hsrtia_t rtia, uint32_t freq) {
	// find previous and next calibration point in frequency
	uint16_t i;
	for (i = 0; i < ARRAY_SIZE(calibration_frequencies) - 1; i++) {
		if (calibration_frequencies[i + 1] >= freq) {
			break;
		}
	}
	auto CalLower = calibration_points[rtia][i];
	auto CalHigher = calibration_points[rtia][i + 1];
	// interpolate calibration values
	Calibration c;
	c.MagCal = interpolate(freq, calibration_frequencies[i],
			calibration_frequencies[i + 1], CalLower.MagCal, CalHigher.MagCal);
	c.PhaseCal = interpolate(freq, calibration_frequencies[i],
			calibration_frequencies[i + 1], CalLower.PhaseCal, CalHigher.PhaseCal);
	return c;
}

static uint32_t GetCalibrationExcitationAmplitude(ad5940_hsrtia_t rtia) {
	switch(rtia) {
	case AD5940_HSRTIA_200:
	case AD5940_HSRTIA_1K:
		// maximum voltage measurable by ADC is 300mV
		return 300000;
	case AD5940_HSRTIA_5K:
		return 60000;
	case AD5940_HSRTIA_10K:
		return 30000;
	case AD5940_HSRTIA_20K:
		return 15000;
	case AD5940_HSRTIA_40K:
		return 7500;
	case AD5940_HSRTIA_80K:
		return 3750;
	case AD5940_HSRTIA_160K:
		return 1875;
	default:
		// should not get here
		return 0;
	}
}

static constexpr uint8_t msgQueueLen = 16;
static uint8_t queueBuf[sizeof(Message) * msgQueueLen];

static bool SetBias(uint32_t biasVoltage) {
	if(biasVoltage > 10000000) {
		return false;
	}
	uint32_t highSide = biasVoltage + referenceVoltage;
	// convert to DAC voltage (Gain of 4.9)
	uint32_t V_DAC = highSide * 10 / 49;
	uint16_t code_DAC = util_Map(V_DAC, 200000, 2400000, 0, 4095);
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_LPDACDAT0, code_DAC, 0x00000FFF);
	ad5940_release_mutex(&ad);
	return true;
}

static void SetSwitchesForRCAL() {
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_SWCON, AD5940_EXAMP_DSW_RCAL0 | AD5940_HSTSW_RCAL1, 0xF00F);
	ad5940_release_mutex(&ad);
}

static void SetSwitchesForMeasurement() {
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_SWCON, AD5940_EXAMP_DSW_CE0 | AD5940_HSTSW_DE0_DIRECT, 0xF00F);
	ad5940_release_mutex(&ad);
}

enum class ADCMeasurement : uint8_t {
	Current,
	Voltage,
	VoltageCalibrationResistor,
};

static void StartADC(ADCMeasurement m) {
	ad5940_ADC_stop(&ad);
	switch(m) {
	case ADCMeasurement::Current:
		ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_HSTIAP, AD5940_ADC_MUXN_HSTIAN);
		break;
	case ADCMeasurement::Voltage:
		ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_AIN0, AD5940_ADC_MUXN_AIN1);
		break;
	case ADCMeasurement::VoltageCalibrationResistor:
		ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_AIN2, AD5940_ADC_MUXN_AIN3);
		break;
	}
	ad5940_ADC_start(&ad);
}

static void SetCalibrationMeasurement(uint32_t freq, ad5940_hsrtia_t rtia) {
	SetSwitchesForRCAL();
	// Configure the frontend
	SetBias(0);
	ad5940_waveinfo_t wave;
	wave.type = AD5940_WAVE_SINE;
	wave.sine.amplitude = GetCalibrationExcitationAmplitude(rtia);
	wave.sine.frequency = freq;
	wave.sine.offset = 0;
	wave.sine.phaseoffset = 0;
	ad5940_generate_waveform(&ad, &wave);
}

static void frontend_task(void*) {
	enum class State : uint8_t {
		Stopped,
		Measuring,
		Calibrating,

	};
	State state = State::Stopped;

	// Measurement variables
	Frontend::Settings settings;
	uint8_t sampleCnt = 0;
	bool voltageMeasurement = false;
	uint16_t excitationAmplitude = 10;
	float sumMagCurrent = 0.0f;
	float sumPhaseCurrent = 0.0f;
	float sumMagVoltage = 0.0f;
	float sumPhaseVoltage = 0.0f;
	ad5940_hsrtia_t rtia = AD5940_HSRTIA_10K;

	// Calibration state variables
	uint16_t calFreqIndex = 0;
	while(1) {
		uint32_t delay = state == State::Stopped ? portMAX_DELAY : 5;
		Message msg;
		if (xQueueReceive(queueHandle, &msg, delay)) {
			// Got message, handle
			switch(msg.type) {
			case MessageType::StopMeasurement:
				state = State::Stopped;
				break;
			case MessageType::RunCalibration:
				calFreqIndex = 0;
				rtia = AD5940_HSRTIA_200;
				state = State::Calibrating;
				calFreqIndex = 0;
				sampleCnt = 0;
				sumMagCurrent = 0.0f;
				sumPhaseCurrent = 0.0f;
				sumMagVoltage = 0.0f;
				sumPhaseVoltage = 0.0f;
				voltageMeasurement = false;
				settings.averages = 100;

				// Configure the frontend
				SetCalibrationMeasurement(calibration_frequencies[calFreqIndex], rtia);

				StartADC(ADCMeasurement::Current);
				break;
			case MessageType::MeasurementConfig:
				settings = msg.settings;
				state = State::Measuring;
				sampleCnt = 0;
				sumMagCurrent = 0.0f;
				sumPhaseCurrent = 0.0f;
				sumMagVoltage = 0.0f;
				sumPhaseVoltage = 0.0f;
				voltageMeasurement = false;

				// Configure the frontend
				SetSwitchesForMeasurement();
				SetBias(settings.biasVoltage);
				ad5940_waveinfo_t wave;
				wave.type = AD5940_WAVE_SINE;
				wave.sine.amplitude = excitationAmplitude;
				wave.sine.frequency = settings.frequency;
				wave.sine.offset = 0;
				wave.sine.phaseoffset = 0;
				ad5940_generate_waveform(&ad, &wave);

				StartADC(ADCMeasurement::Current);

				break;
			}
		}
		switch(state) {
		case State::Stopped:
			// nothing to do
			break;
		case State::Calibrating:
		case State::Measuring: {
			ad5940_dftresult_t result;
			ad5940_get_dft_result(&ad, 1, &result);
			sampleCnt++;
			if (sampleCnt > 1) {
				if (voltageMeasurement) {
					sumMagVoltage += result.mag;
					sumPhaseVoltage += result.phase;
				} else {
					sumMagCurrent += result.mag;
					sumPhaseCurrent += result.phase;
				}
			}
			if (sampleCnt > settings.averages + 1) {
				if (voltageMeasurement) {
					// all done calculate impedance
					sumMagCurrent /= settings.averages;
					sumPhaseCurrent /= settings.averages;
					sumMagVoltage /= settings.averages;
					sumPhaseVoltage /= settings.averages;
					float mag = sumMagVoltage / sumMagCurrent;
					float phase = sumPhaseVoltage - sumPhaseCurrent;
					sumMagCurrent = 0.0f;
					sumPhaseCurrent = 0.0f;
					sumMagVoltage = 0.0f;
					sumPhaseVoltage = 0.0f;

					if (state == State::Measuring) {
						auto cal = GetCalibration(rtia, settings.frequency);
						mag *= cal.MagCal;
						phase -= cal.PhaseCal;
						Frontend::Result result;
						result.Magnitude = mag;
						result.Phase = phase;
						result.I = mag * cos(phase);
						result.Q = mag * sin(phase);
						// TODO fill with proper values
						result.range = Frontend::Range::AUTO;
						result.valid = true;
						if (callback) {
							callback(cb_ctx, result);
						}
					} else if (state == State::Calibrating) {
						// Store in appopriate calibration slot (calibration resistor is 1k)
						calibration_points[rtia][calFreqIndex].MagCal = 1000.0f
								/ mag;
						calibration_points[rtia][calFreqIndex].PhaseCal = phase;
						// move on to next step
						if (calFreqIndex < ARRAY_SIZE(calibration_frequencies)) {
							calFreqIndex++;
						} else {
							calFreqIndex = 0;
							if (rtia != AD5940_HSRTIA_160K) {
								rtia = (ad5940_hsrtia_t) (uint8_t) (rtia + 1);
							} else {
								// Calibration routine complete
								state = State::Stopped;
								break;
							}
						}
						SetCalibrationMeasurement(
								calibration_frequencies[calFreqIndex], rtia);
					}

					voltageMeasurement = false;
					StartADC(ADCMeasurement::Current);
				} else {
					voltageMeasurement = true;
					StartADC(ADCMeasurement::Voltage);
				}
				sampleCnt = 0;
			}
		}
			break;
		}
	}
}

bool Frontend::Init() {
	Persistence::Add(calibration_points, sizeof(calibration_points));
	ad.CSport = AD5941_CS_GPIO_Port;
	ad.CSpin = AD5941_CS_Pin;
	ad.spi = &hspi3;
	if(ad5940_init(&ad) != AD5940_RES_OK) {
		LOG(Log_Frontend, LevelError, "AD5941 initalization failed");
		return false;
	}

	// connect 12bit DAC to PA and PA to RE0 (sets bias voltage)
	ad5940_take_mutex(&ad);
	// close SW4 of low power DAC
	ad5940_set_bits(&ad, AD5940_REG_LPDACSW0, 0x30);
	// close SW15 and SW8 of low power TIA (low power DAC buffer mode + output enable to RE0)
	ad5940_write_reg(&ad, AD5940_REG_LPTIASW0, 0x8100);
	// power up PA
	ad5940_clear_bits(&ad, AD5940_REG_LPTIACON0, 0x02);

	SetBias(0);

	// Set AD5941 to high power mode
	ad5940_write_reg(&ad, AD5940_REG_PMBW, 0x000D);
	// Set system clock divider to 2
	// unlock clock con0
	ad5940_write_reg(&ad, AD5940_REG_CLKCON0KEY, 0xA815);
	ad5940_write_reg(&ad, AD5940_REG_CLKCON0, 2);
	// Lock clock con0
	ad5940_write_reg(&ad, AD5940_REG_CLKCON0KEY, 0);
	// Switch system and ADC clock to external crystal
	ad5940_write_reg(&ad, AD5940_REG_CLKSEL, 0x05);
	// Enable 1.6MHz sample rate
	ad5940_set_bits(&ad, AD5940_REG_ADCFILTERCON, 0x01);
	ad5940_release_mutex(&ad);

	// Configure TIA and excitation amplifier
	ad5940_set_HSTIA(&ad, AD5940_HSRTIA_200, 2, AD5940_HSTIA_VBIAS_1V1, true,
			AD5940_HSTSW_RCAL1);
	ad5940_set_excitation_amplifier(&ad, AD5940_EXAMP_DSW_RCAL0,
			AD5940_EXAMP_PSW_INT_FEEDBACK, AD5940_EXAMP_NSW_INT_FEEDBACK,
			false);

	ad5940_set_PGA_gain(&ad, AD5940_PGA_GAIN_4);

	// Configure DFT
	ad5940_dftconfig_t dft;
	dft.hanning = true;
	dft.points = 16384;
	dft.source = AD5940_DFTSRC_RAW;
	ad5940_set_dft(&ad, &dft);

	// Start task
	taskHandle = xTaskCreateStatic(frontend_task, "Frontend", stack_size_words,
			nullptr, 3, task_stack, &task);

	queueHandle = xQueueGenericCreateStatic(msgQueueLen, sizeof(Message),
			queueBuf, &msgQueue, 0);

	return true;
}

void Frontend::SetCallback(Callback cb, void *ctx) {
	callback = cb;
	cb_ctx = ctx;
}

bool Frontend::Stop() {
	Message msg;
	msg.type = MessageType::StopMeasurement;
	return xQueueSend(queueHandle, &msg, 0) == pdPASS;
}

bool Frontend::Start(Settings s) {
	Message msg;
	msg.type = MessageType::MeasurementConfig;
	msg.settings = s;
	return xQueueSend(queueHandle, &msg, 0) == pdPASS;
}
