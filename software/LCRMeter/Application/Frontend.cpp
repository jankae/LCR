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
#include "GUI/Dialog/progress.hpp"
#include "HardwareLimits.hpp"
#include "gui.hpp"

static ad5940_t ad;
extern SPI_HandleTypeDef hspi3;

#define Log_Frontend (LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

static constexpr uint32_t referenceVoltage = 1100000;
static Frontend::Callback callback;
static void *cb_ctx;
static ProgressBar *AcquisitionProgress = nullptr;

static constexpr ad5940_pga_gain_t PGA_gain = AD5940_PGA_GAIN_4;

static constexpr uint32_t stack_size_words = 1024;
static uint32_t task_stack[stack_size_words];
static TaskHandle_t taskHandle;
static StaticTask_t task;
static StaticQueue_t msgQueue;
static QueueHandle_t queueHandle;

static ProgressDialog *cal_dialog;

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
static int32_t calibration_BiasVoltageOffset;

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

/*
 * Sets the ADC averages depending on the selected frequency in order
 * to cover at least a few waveform periods with the DFT
 */
static void SetADCAverages(uint32_t freq) {
	uint32_t rawSamplesPerPeriod = HardwareLimits::ADCSampleRate / freq;
	constexpr uint8_t MinPeriodsPerDFT = 10;
	uint16_t avgRegVal = 0x0000;
	if (HardwareLimits::DFTpoints / (rawSamplesPerPeriod / 2)
			>= MinPeriodsPerDFT) {
		// Averaging of 2 is enough
		avgRegVal = 0x0000;
		LOG(Log_Frontend, LevelDebug, "ADC averaging: 2");
	} else if (HardwareLimits::DFTpoints / (rawSamplesPerPeriod / 4)
			>= MinPeriodsPerDFT) {
		// Averaging of 4 is enough
		avgRegVal = 0x4000;
		LOG(Log_Frontend, LevelDebug, "ADC averaging: 4");
	} else if (HardwareLimits::DFTpoints / (rawSamplesPerPeriod / 8)
			>= MinPeriodsPerDFT) {
		// Averaging of 8 is enough
		avgRegVal = 0x8000;
		LOG(Log_Frontend, LevelDebug, "ADC averaging: 8");
	} else {
		// needs maximum averaging of 16
		avgRegVal = 0xC000;
		LOG(Log_Frontend, LevelDebug, "ADC averaging: 16");
	}
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_ADCFILTERCON, avgRegVal, 0xC000);
	ad5940_release_mutex(&ad);
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

static bool SetBias(int32_t biasVoltage, bool applyCalibration = true) {
	if(biasVoltage > 10000000) {
		return false;
	}
	uint32_t highSide = biasVoltage + referenceVoltage;
	if (applyCalibration) {
		highSide += calibration_BiasVoltageOffset;
	}
	// convert to DAC voltage (Gain of 4.9)
	uint32_t V_DAC = highSide * 10 / 49;
	uint16_t code_DAC = util_Map(V_DAC, 200000, 2400000, 0, 4095);
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_LPDACDAT0, code_DAC, 0x00000FFF);
	ad5940_release_mutex(&ad);
	LOG(Log_Frontend, LevelDebug, "Set bias voltage of %ld, DAC code %u", biasVoltage, code_DAC);
	return true;
}

static void SetSwitchesForRCAL() {
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_SWCON, AD5940_EXAMP_DSW_RCAL0 | AD5940_HSTSW_RCAL1, 0xF00F);
	ad5940_release_mutex(&ad);
	LOG(Log_Frontend, LevelDebug, "Switches set for calibration");
}

static void SetSwitchesForMeasurement() {
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_SWCON, AD5940_EXAMP_DSW_CE0 | AD5940_HSTSW_DE0_DIRECT, 0xF00F);
	ad5940_release_mutex(&ad);
	LOG(Log_Frontend, LevelDebug, "Switches set for measurement");
}

enum class ADCMeasurement : uint8_t {
	Current,
	Voltage,
	VoltageCalibrationResistor,
};

static void StartADC(ADCMeasurement m) {
	ad5940_ADC_stop(&ad);
	// Clear ADC min/max flags
	ad5940_take_mutex(&ad);
	ad5940_write_reg(&ad, AD5940_REG_INTCCLR, 0x30);
	ad5940_release_mutex(&ad);
	switch(m) {
	case ADCMeasurement::Current:
		ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_HSTIAP, AD5940_ADC_MUXN_HSTIAN);
		LOG(Log_Frontend, LevelDebug, "Starting ADC current measurement");
		break;
	case ADCMeasurement::Voltage:
		ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_AIN1, AD5940_ADC_MUXN_AIN0);
		LOG(Log_Frontend, LevelDebug, "Starting ADC voltage measurement");
		break;
	case ADCMeasurement::VoltageCalibrationResistor:
		ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_AIN2, AD5940_ADC_MUXN_AIN3);
		LOG(Log_Frontend, LevelDebug, "Starting ADC calibration measurement");
		break;
	}
	ad5940_ADC_start(&ad);
}

static void SetCalibrationMeasurement(uint32_t freq, ad5940_hsrtia_t rtia) {
	SetSwitchesForRCAL();
	// Configure the frontend
	SetBias(0);
	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_HSRTIACON, rtia, 0x0F);
	ad5940_release_mutex(&ad);
	ad5940_waveinfo_t wave;
	wave.type = AD5940_WAVE_SINE;
	wave.sine.amplitude = GetCalibrationExcitationAmplitude(rtia);
	wave.sine.frequency = freq * 1000;
	wave.sine.offset = 0;
	wave.sine.phaseoffset = 0;
	ad5940_generate_waveform(&ad, &wave);
	SetADCAverages(freq);
}

static uint32_t GetADCAverage(uint16_t samples) {
	uint32_t sum = 0;
	ad5940_take_mutex(&ad);
	for (uint16_t i = 0; i < samples; i++) {
		sum += ad5940_read_reg(&ad, AD5940_REG_ADCDAT);
		vTaskDelay(1);
	}
	ad5940_release_mutex(&ad);
	return sum / samples;
}

// Assumes the outputs are shorted
static void RunBiasVoltageCalibration() {
	// First, get ADC offset
	ad5940_ADC_stop(&ad);
	// Connect +/- inputs to same voltage
	ad5940_set_ADC_mux(&ad, AD5940_ADC_MUXP_VBIAS_CAP, AD5940_ADC_MUXN_VBIAS_CAP);
	ad5940_ADC_start(&ad);
	int32_t ADCoffset = GetADCAverage(100);
	LOG(Log_Frontend, LevelDebug, "ADC offset: %ld", ADCoffset);
	// Reasonable limits for worst case bias offset calibration
	constexpr int32_t minBias = -100000;
	constexpr int32_t maxBias = 100000;
	// bias voltage change equivalent to smaller 0.5LSB of DAC
	constexpr int32_t smallestChange = 1000;

	// Set waveform amplitude to zero, highest amplification for TIA
	ad5940_waveinfo_t wave;
	wave.type = AD5940_WAVE_SINE;
	wave.sine.amplitude = 0;
	wave.sine.frequency = 1000000;
	wave.sine.offset = 0;
	wave.sine.phaseoffset = 0;
	ad5940_generate_waveform(&ad, &wave);

	ad5940_take_mutex(&ad);
	ad5940_modify_reg(&ad, AD5940_REG_HSRTIACON, AD5940_HSRTIA_160K, 0x0F);
	ad5940_release_mutex(&ad);

	SetSwitchesForMeasurement();
	StartADC(ADCMeasurement::Current);

	// binary search for bias voltage that results in the least current
	int32_t step_size = (maxBias - minBias) / 4;
	calibration_BiasVoltageOffset = 0;
	do {
		// Set new bias voltage
		SetBias(calibration_BiasVoltageOffset, false);
		// Wait for voltage to settle
		vTaskDelay(1000);
		// Sample ADC. Due to high gain of TIA even small voltage differences
		// result in high current -> low number of averages sufficient
		int32_t adc = GetADCAverage(10);

		LOG(Log_Frontend, LevelDebug, "Bias Cal: %ld -> ADC: %ld",
				calibration_BiasVoltageOffset, adc - ADCoffset);
		if (adc < ADCoffset) {
			// positive current into DE0, bias voltage too high
			calibration_BiasVoltageOffset -= step_size;
		} else {
			// negative current into DE0, bias voltage too low
			calibration_BiasVoltageOffset += step_size;
		}
		// reduce step size for next iteration
		step_size /= 2;
	} while(step_size > smallestChange);
	SetBias(0);
}

static void UpdateAcquisitionState(uint8_t percentage) {
	if(AcquisitionProgress) {
		AcquisitionProgress->setState(percentage);
		GUIEvent_t ev;
		ev.type = EVENT_NONE;
		GUI::SendEvent(&ev);
	}
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
	float sumMagCurrent = 0.0f;
	float sumPhaseCurrent = 0.0f;
	float sumMagVoltage = 0.0f;
	float sumPhaseVoltage = 0.0f;
	ad5940_hsrtia_t rtia = AD5940_HSRTIA_1K;
	bool currentMeasurementClipped = false;
	uint32_t averagesBuffer;

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
				// Create calibration window
				cal_dialog = new ProgressDialog("Calibrating...", 200);
				RunBiasVoltageCalibration();
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
				averagesBuffer = settings.averages;
				settings.averages = 50;

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

				if (settings.range == Frontend::Range::Lowest) {
					rtia = AD5940_HSRTIA_200;
				} else if (settings.range == Frontend::Range::Highest) {
					rtia = AD5940_HSRTIA_160K;
				}

				// Configure the frontend
				SetSwitchesForMeasurement();
				SetBias(settings.biasVoltage);
				// Select correct TIA gain
				ad5940_take_mutex(&ad);
				ad5940_modify_reg(&ad, AD5940_REG_HSRTIACON, rtia, 0x0F);
				ad5940_release_mutex(&ad);
				ad5940_waveinfo_t wave;
				wave.type = AD5940_WAVE_SINE;
				wave.sine.amplitude = msg.settings.excitationVoltage;
				wave.sine.frequency = settings.frequency * 1000;
				wave.sine.offset = 0;
				wave.sine.phaseoffset = 0;
				ad5940_generate_waveform(&ad, &wave);
				SetADCAverages(settings.frequency);

				StartADC(ADCMeasurement::Current);
				UpdateAcquisitionState(0);

				break;
			}
			if(msg.type != MessageType::RunCalibration && cal_dialog) {
				delete cal_dialog;
				cal_dialog = nullptr;
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
//			ad5940_take_mutex(&ad);
//			int32_t adc = ad5940_read_reg(&ad, AD5940_REG_ADCDAT);
//			ad5940_release_mutex(&ad);
//			LOG(Log_Frontend, LevelDebug, "Raw ADC: %u", adc);
			sampleCnt++;
			// Each measurement (current/voltage takes 50 percent of the acquisition time)
			uint8_t acquisitionPercentage = util_Map(sampleCnt, 0,
					settings.averages + 1, 0, 50);
			if (voltageMeasurement) {
				acquisitionPercentage += 50;
			}
			UpdateAcquisitionState(acquisitionPercentage);
			if (sampleCnt > 1) {
				if (voltageMeasurement) {
					sumMagVoltage += result.mag;
					sumPhaseVoltage += result.phase;
				} else {
					if (state == State::Measuring
							&& settings.range == Frontend::Range::AUTO) {
						// check clipping and change range before averaging is complete -> faster range switching
						ad5940_take_mutex(&ad);
						currentMeasurementClipped = ad5940_read_reg(&ad,
								AD5940_REG_INTCFLAG0) & 0x30;
						ad5940_release_mutex(&ad);
						if (currentMeasurementClipped) {
							// reduce tia gain if possible
							if (rtia != AD5940_HSRTIA_200) {
								rtia = (ad5940_hsrtia_t) ((int) rtia - 1);
								ad5940_take_mutex(&ad);
								ad5940_modify_reg(&ad, AD5940_REG_HSRTIACON,
										rtia, 0x0F);
								ad5940_release_mutex(&ad);
								// reset sample cnt and restart ADC (resets clipped bits)
								sampleCnt = 0;
								StartADC(ADCMeasurement::Current);
							}
						}
					}
					sumMagCurrent += result.mag;
					sumPhaseCurrent += result.phase;
				}
			}
			if (sampleCnt > settings.averages) {
				if (voltageMeasurement) {
					Frontend::Result result;
					// all done calculate impedance
					sumMagCurrent /= settings.averages;
					sumPhaseCurrent /= settings.averages;
					sumMagVoltage /= settings.averages;
					sumPhaseVoltage /= settings.averages;

					ad5940_take_mutex(&ad);
					bool voltageMeasurementClipped = ad5940_read_reg(&ad, AD5940_REG_INTCFLAG0) & 0x30;
					ad5940_release_mutex(&ad);

					constexpr uint32_t ADC_max_value = 32768		// 16384 is the value according to datasheet, ADC actually reports values up to 32768
														* 4;		// two additional bits due to DFT
					float rangeU = sumMagVoltage * (1UL << 17) / ADC_max_value;
					float rangeI = sumMagCurrent * (1UL << 17) / ADC_max_value;
					// multiplication by 120 instead of 100 to have some headroom above displayed ADC ranges
					result.usedRangeU = rangeU * 120;
					result.usedRangeI = rangeI * 120;

					// Check ranges for valid result
					Frontend::ResultType type = Frontend::ResultType::Valid;
					constexpr float minRangeU = 0.00005f;
					constexpr float minRangeI = 0.02f;
					if (rangeI < minRangeI && rangeU < minRangeU) {
						// No current flowing and no voltage measured -> no leads connected
						type = Frontend::ResultType::OpenLeads;
					} else if (rangeI < minRangeI || voltageMeasurementClipped) {
						// No current flowing, connected impedance too high
						type = Frontend::ResultType::Overrange;
					} else if (rangeU < minRangeU || currentMeasurementClipped) {
						// No voltage measurable, impedance too low
						type = Frontend::ResultType::Underrange;
					}

					// Adjust current measurement by TIA gain (results in all calibration factors roughly equal to 1)
					sumMagCurrent /= ad5940_HSTIA_gain_to_value(rtia);
					float mag = sumMagVoltage / sumMagCurrent;
					float phase = sumPhaseVoltage - sumPhaseCurrent;
					LOG(Log_Frontend, LevelDebug,
							"Measurement U: %f@%f, I: %f@%f", sumMagVoltage,
							sumPhaseVoltage, sumMagCurrent, sumPhaseCurrent);

					if (state == State::Measuring) {
						auto cal = GetCalibration(rtia, settings.frequency);
						mag *= cal.MagCal;
						phase -= cal.PhaseCal;
						// constrain phase to +/-PI
						if (phase >= M_PI) {
							phase -= 2 * M_PI;
						} else if (phase <= -M_PI) {
							phase += 2 * M_PI;
						}
//						result.Magnitude = mag;
//						// convert phase to degrees
//						result.Phase = phase * 180.0 / M_PI;
						/*
						 * Reconstruct I/Q values from magnitude and phase, making it easier to calculate
						 * component values later on. Original I/Q is not valid anymore because the
						 * calibration works in the polar domain
						 */
						result.Z = std::complex<float>(mag * cos(phase), mag * sin(phase));
						result.frequency = settings.frequency;

						constexpr float mag_to_RMS = (1UL << 17)		// compensate division in ad5940_get_dft_result
													* 1.835f/32768		// ADC bits and slope compensation
													* 0.25f 			// Compensate additional 2 bits in DFT
													* M_SQRT1_2;		// Peak to RMS
						result.RMS_U = sumMagVoltage * mag_to_RMS * 10 / ad5940_PGA_gain_to_value10(PGA_gain);
						result.RMS_I = sumMagCurrent * mag_to_RMS * 10 / ad5940_PGA_gain_to_value10(PGA_gain);
						/*
						 * The calibration factor contains corrections for both the current and the voltage, it is
						 * not possible to separate their influences. However, the voltage accuracy of the AD5941 is
						 * significantly better than the current accuracy (due to inaccurate resistors in the TIA) so
						 * it is assumed here that all error sources come from the current measurement only and the
						 * calibration factor is used to correct the measured current value.
						 */
						result.RMS_I /= cal.MagCal;

						// Calculate lowest and highest measurable impedances
						float smallestCurrent = result.RMS_I * minRangeI / rangeI;
						float smallestVoltage = result.RMS_U * minRangeU / rangeU;
						float highestCurrent = result.RMS_I / rangeI;
						float highestVoltage = (float) settings.excitationVoltage
												* 0.000001		// Convert from uV to V
												* M_SQRT1_2;	// Convert from peak to RMS
						if (result.clippedI) {
							result.LimitLow = smallestVoltage / highestCurrent;
						} else {
							result.LimitLow = smallestVoltage / result.RMS_I;
						}
						if (result.clippedU) {
							result.LimitHigh = highestVoltage / smallestCurrent;
						} else {
							result.LimitHigh = result.RMS_U / smallestCurrent;
						}

						result.type = type;
						result.clippedI = currentMeasurementClipped;
						result.clippedU = voltageMeasurementClipped;

						// TODO fill with proper values
						result.range = Frontend::Range::AUTO;
						UpdateAcquisitionState(0);
						if (settings.range == Frontend::Range::AUTO) {
							// Check if range switch is required
							if (result.clippedI || result.usedRangeI > 95) {
								// reduce tia gain if possible
								if (rtia != AD5940_HSRTIA_200) {
									rtia = (ad5940_hsrtia_t) ((int)rtia - 1);
									ad5940_take_mutex(&ad);
									ad5940_modify_reg(&ad, AD5940_REG_HSRTIACON,
											rtia, 0x0F);
									ad5940_release_mutex(&ad);
									result.type = Frontend::ResultType::Ranging;
								}
							} else if (result.usedRangeI < 15) {
								// increase tia gain if possible
								if (rtia != AD5940_HSRTIA_160K) {
									rtia = (ad5940_hsrtia_t) ((int)rtia + 1);
									ad5940_take_mutex(&ad);
									ad5940_modify_reg(&ad, AD5940_REG_HSRTIACON,
											rtia, 0x0F);
									ad5940_release_mutex(&ad);
									result.type = Frontend::ResultType::Ranging;
								}
							}
						}
						if (callback) {
							callback(cb_ctx, result);
						}
					} else if (state == State::Calibrating) {
						// Store in appropriate calibration slot (calibration resistor is 1k5)
						float magCal = 1000.0f / mag;
						LOG(Log_Frontend, LevelInfo,
								"Calibration at gain %lu, frequency %luHz is: %f@%f",
								ad5940_HSTIA_gain_to_value(rtia),
								calibration_frequencies[calFreqIndex], magCal,
								phase);
						calibration_points[rtia][calFreqIndex].MagCal = magCal;
						calibration_points[rtia][calFreqIndex].PhaseCal = phase;
						// calculate percentage of calibration
						uint8_t percentage = 100UL * (rtia
								* ARRAY_SIZE(calibration_frequencies)
								+ calFreqIndex)
								/ (AD5940_HSRTIA_OPEN
										* ARRAY_SIZE(calibration_frequencies));
						if (cal_dialog) {
							cal_dialog->SetPercentage(percentage);
						}
						// move on to next step
						if (calFreqIndex < ARRAY_SIZE(calibration_frequencies) - 1) {
							calFreqIndex++;
						} else {
							calFreqIndex = 0;
							if (rtia != AD5940_HSRTIA_160K) {
								rtia = (ad5940_hsrtia_t) (uint8_t) (rtia + 1);
							} else {
								// Calibration routine complete
								state = State::Stopped;
								if (cal_dialog) {
									delete cal_dialog;
									cal_dialog = nullptr;
									Persistence::Save();
								}
								// Start again with measurement
								settings.averages = averagesBuffer;
								Frontend::Start(settings);
								break;
							}
						}
						SetCalibrationMeasurement(
								calibration_frequencies[calFreqIndex], rtia);
					}

					sumMagCurrent = 0.0f;
					sumPhaseCurrent = 0.0f;
					sumMagVoltage = 0.0f;
					sumPhaseVoltage = 0.0f;
					voltageMeasurement = false;
					StartADC(ADCMeasurement::Current);
				} else {
					voltageMeasurement = true;
					// Check ADC limits of the finished current measurement
					ad5940_take_mutex(&ad);
					currentMeasurementClipped = ad5940_read_reg(&ad, AD5940_REG_INTCFLAG0) & 0x30;
					ad5940_release_mutex(&ad);
					if (state == State::Calibrating) {
						StartADC(ADCMeasurement::VoltageCalibrationResistor);
					} else {
						StartADC(ADCMeasurement::Voltage);
					}
				}

				sampleCnt = 0;
			}
		}
			break;
		}
	}
}

bool Frontend::Init() {
	// Release reset
	AD5941_RESET_GPIO_Port->BSRR = AD5941_RESET_Pin;
	Persistence::Add(calibration_points, sizeof(calibration_points));
	Persistence::Add(&calibration_BiasVoltageOffset, sizeof(calibration_BiasVoltageOffset));
	// Set calibration to default values in case of missing persistence data
	for (uint8_t i = 0; i < AD5940_HSRTIA_OPEN; i++) {
		for (uint8_t j = 0; j < ARRAY_SIZE(calibration_frequencies); j++) {
			calibration_points[i][j].MagCal = 1.0f;
			calibration_points[i][j].PhaseCal = 0.0f;
		}
	}
	calibration_BiasVoltageOffset = 0;
	ad.CSport = AD5941_CS_GPIO_Port;
	ad.CSpin = AD5941_CS_Pin;
	ad.spi = &hspi3;
	vTaskDelay(5);
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
	ad5940_modify_reg(&ad, AD5940_REG_PMBW, 0x000D, 0x000F);
	LOG(Log_Frontend, LevelDebug, "CLKCON0 before writing to it: 0x%04x", ad5940_read_reg(&ad, AD5940_REG_CLKCON0));
	// unlock clock con0
	ad5940_write_reg(&ad, AD5940_REG_CLKCON0KEY, 0xA815);
	// Set system clock and ADC control clock divider to 2
	ad5940_modify_reg(&ad, AD5940_REG_CLKCON0, 0x0042, 0x03FF);
	// Lock clock con0
	ad5940_write_reg(&ad, AD5940_REG_CLKCON0KEY, 0);
	LOG(Log_Frontend, LevelDebug, "CLKCON0 after writing to it: 0x%04x", ad5940_read_reg(&ad, AD5940_REG_CLKCON0));
	// Enable external crystal oscillator
	// unlock osccon0
	ad5940_write_reg(&ad, AD5940_REG_OSCKEY, 0xCB14);
	ad5940_set_bits(&ad, AD5940_REG_OSCCON, 0x04);
	// lock osccon0
	ad5940_write_reg(&ad, AD5940_REG_OSCKEY, 0);
	vTaskDelay(5);
	uint16_t osccon = ad5940_read_reg(&ad, AD5940_REG_OSCCON);
	if(!(osccon & 0x0400)) {
		LOG(Log_Frontend, LevelError, "External crystal failed to start");
		return false;
	}

	// Switch system and ADC clock to external crystal
	ad5940_modify_reg(&ad, AD5940_REG_CLKSEL, 0x05, 0x000F);
	// Enable 1.6MHz sample rate
	ad5940_clear_bits(&ad, AD5940_REG_ADCFILTERCON, 0x01);

	/*
	 * Setup clipping limits for ADC:
	 * The min/max flag is set if an ADC sample gets too close to the ADC limits.
	 * The hysteresis is set up so that the flag is never reset and thus preserving
	 * even a single clipped ADC value.
	 */
	ad5940_write_reg(&ad, AD5940_REG_ADCMIN, HardwareLimits::ADCHeadroom);
	ad5940_write_reg(&ad, AD5940_REG_ADCMINSM, UINT16_MAX - HardwareLimits::ADCHeadroom);
	ad5940_write_reg(&ad, AD5940_REG_ADCMAX, UINT16_MAX - HardwareLimits::ADCHeadroom);
	ad5940_write_reg(&ad, AD5940_REG_ADCMAXSMEN, UINT16_MAX - HardwareLimits::ADCHeadroom);
	// enable min/max interrupts
	ad5940_set_bits(&ad, AD5940_REG_INTCSEL0, 0x30);

	// bypass SINC3 filter
	ad5940_set_bits(&ad, AD5940_REG_ADCFILTERCON, 1UL << 6);

	// Set recommended DAC update rate
	ad5940_modify_reg(&ad, AD5940_REG_HSDACCON, 0x000E, 0x01FE);

	ad5940_release_mutex(&ad);

	// Configure TIA and excitation amplifier
	ad5940_set_HSTIA(&ad, AD5940_HSRTIA_200, 0, AD5940_HSTIA_VBIAS_1V1, true,
			AD5940_HSTSW_RCAL1);
	ad5940_set_excitation_amplifier(&ad, AD5940_EXAMP_DSW_RCAL0,
			AD5940_EXAMP_PSW_INT_FEEDBACK, AD5940_EXAMP_NSW_INT_FEEDBACK,
			false);

	ad5940_set_PGA_gain(&ad, PGA_gain);

	// Configure DFT
	ad5940_dftconfig_t dft;
	dft.hanning = true;
	dft.points = HardwareLimits::DFTpoints;
	dft.source = AD5940_DFTSRC_AVG;
	ad5940_set_dft(&ad, &dft);

	queueHandle = xQueueGenericCreateStatic(msgQueueLen, sizeof(Message),
			queueBuf, &msgQueue, 0);

	// Start task
	taskHandle = xTaskCreateStatic(frontend_task, "Frontend", stack_size_words,
			nullptr, 4, task_stack, &task);

	return true;
}

void Frontend::SetCallback(Callback cb, void *ctx) {
	callback = cb;
	cb_ctx = ctx;
}

void Frontend::SetAcquisitionProgressBar(ProgressBar *p) {
	AcquisitionProgress = p;
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

bool Frontend::Calibrate() {
	Message msg;
	msg.type = MessageType::RunCalibration;
	return xQueueSend(queueHandle, &msg, 0) == pdPASS;
}
