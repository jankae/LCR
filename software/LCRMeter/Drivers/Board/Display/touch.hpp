#ifndef TOUCH_H_
#define TOUCH_H_

#include <util.h>
#include "display.h"
#include "exti.h"

#define TOUCH_RESOLUTION_X		DISPLAY_WIDTH
#define TOUCH_RESOLUTION_Y		DISPLAY_HEIGHT

namespace Touch {

void Init(void);
bool GetCoordinates(coords_t &c);
bool SetPENCallback(exti_callback_t cb, void *ptr);
bool ClearPENCallback(void);
}

#endif
