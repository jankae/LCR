#pragma once
#include "color.h"

namespace LCR {

constexpr color_t SchematicColor = COLOR_DARKGRAY;
constexpr color_t MeasurmentValueColor = COLOR_BLACK;
constexpr color_t BarColor = COLOR(11, 68, 181);

bool Init();
void Run();

}
