#pragma once
#include <stdint.h>
#include "progressbar.hpp"
#include <complex>

namespace Frontend {

enum class Range : uint8_t {
	AUTO,
	Lowest,
	Highest,
};

enum class ResultType : uint8_t {
	Valid,		// Valid result in achieved in the correct range
	Ranging,	// Got a result but the range in auto mode has not settled yet
	Overrange,	// Impedance too high and unable to change range (either not in auto mode or already on highest range)
	Underrange, // Impedance too low and unable to change range (either not in auto mode or already on lowest range)
	OpenLeads,	// no current and no voltage detected -> leads not connected
};

using Result = struct result {
	std::complex<float> Z;
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
