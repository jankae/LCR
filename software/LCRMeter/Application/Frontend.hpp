#pragma once
#include <stdint.h>
#include "progressbar.hpp"

namespace Frontend {

enum class Range : uint8_t {
	AUTO,
};

enum class ResultType : uint8_t {
	Valid,
	Overrange,
	Underrange,
	OpenLeads,
};

using Result = struct result {
	float I, Q;
	float Magnitude;
	float Phase;
	// RMS values of current and voltage
	float RMS_I, RMS_U;
	// percentage of used ADC range according to DFT result (will not be accurate in case of clipping)
	uint8_t usedRangeI, usedRangeU;
	// Indicates whether any sample in the current/voltage measurement exceeded the ADC range
	bool clippedI, clippedU;
	// Min/max measurable impedance in this range
	float LimitLow, LimitHigh;
	ResultType type;
	Range range;
	uint32_t frequency;
};

using Settings = struct settings {
	uint32_t biasVoltage;
	uint32_t frequency;
	uint32_t excitationVoltage;
	Range range;
	uint32_t averages;
};

using Callback = void(*)(void*ctx, Result);

bool Init();
void SetCallback(Callback cb, void *ctx=nullptr);
void SetAcquisitionProgressBar(ProgressBar *p);
bool Stop();
bool Start(Settings s);
bool Calibrate();

}
