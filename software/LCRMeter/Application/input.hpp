#ifndef INPUT_H_
#define INPUT_H_

#include <util.h>
#include <cstdint>
#include "events.hpp"

namespace Input {

constexpr uint32_t LongTouchTime = 1500;

bool Init();
void Calibrate();
bool LoadCalibration();
}


#endif
