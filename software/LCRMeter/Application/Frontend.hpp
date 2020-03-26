#pragma once
#include <stdint.h>

namespace Frontend {

enum class Range : uint8_t {
	AUTO,
};

using Result = struct result {
	float I, Q;
	float Magnitude;
	float Phase;
	Range range;
	bool valid;
	uint32_t frequency;
};

using Settings = struct settings {
	uint32_t biasVoltage;
	uint32_t frequency;
	Range range;
	uint32_t averages;
};

using Callback = void(*)(void*ctx, Result);

bool Init();
void SetCallback(Callback cb, void *ctx=nullptr);
bool Stop();
bool Start(Settings s);

}
