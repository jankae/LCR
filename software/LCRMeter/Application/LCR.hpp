#pragma once
#include "color.h"
#include "Frontend.hpp"
#include <complex>

namespace LCR {

enum class ImpedanceType : uint8_t {
	CAPACITANCE = 0x00,
	INDUCTANCE = 0x01,
};

enum class DisplayMode : uint8_t {
			AUTO = 0x00,
			SERIES = 0x01,
			PARALLEL = 0x02,
};

using Result = struct _result {
	Frontend::Result frontend;
	std::complex<float> Z;
	DisplayMode mode;
	ImpedanceType type;
	union {
		struct {
			float capacitance;
		} C;
		struct {
			float inductance;
		} L;
	};
	// for both Ls and Cs
	float qualityFactor;
};


constexpr color_t SchematicColor = COLOR_DARKGRAY;
constexpr color_t MeasurmentValueColor = COLOR_BLACK;
constexpr color_t BarColor = COLOR(11, 68, 181);

bool Init();
void Run();

}
