#pragma once
#include <cstdint>

namespace HardwareLimits {

static constexpr uint32_t MinFrequency = 100;
static constexpr uint32_t MaxFrequency = 200000;
static constexpr uint32_t MinBiasVoltage = 0;
static constexpr uint32_t MaxBiasVoltage = 10000000;
static constexpr uint32_t MinExcitationVoltage = 10000;
static constexpr uint32_t MaxExcitationVoltage = 600000;
static constexpr uint32_t ADCHeadroom = 1024;
static constexpr uint32_t ADCSampleRate = 1600000;
static constexpr uint32_t DFTpoints = 16384;

}
