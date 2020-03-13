#include "slider.hpp"

Slider::Slider(int32_t* value, int32_t min, int32_t max, coords_t size) {
	this->value = value;
	this->min = min;
	this->max = max;
	this->size = size;
	cb = nullptr;
	cbptr = nullptr;
}

Slider::~Slider() {
}

void Slider::draw(coords_t offset) {
    /* calculate corners */
    coords_t upperLeft = offset;
    coords_t lowerRight = upperLeft;
    lowerRight.x += size.x - 1;
    lowerRight.y += size.y - 1;

    bool vertical = size.y > size.x ? true : false;
    int16_t halfWidth = vertical ? size.x / 2 : size.y / 2;

    coords_t sliderStart = upperLeft;
    sliderStart.x += halfWidth;
    sliderStart.y += halfWidth;
    coords_t sliderStop = lowerRight;
    sliderStop.x -= halfWidth;
    sliderStop.y -= halfWidth;

	if (selectable) {
		display_SetForeground(Border);
	} else {
		display_SetForeground(BorderUnselectable);
	}
	display_Rectangle(sliderStart.x - 3, sliderStart.y - 3, sliderStop.x + 3,
			sliderStop.y + 3);

    // calculate position of knob
	coords_t knob;
	if (vertical) {
		knob.x = sliderStart.x;
		knob.y = util_Map(*value, min, max, sliderStart.y, sliderStop.y);
	} else {
		knob.x = util_Map(*value, min, max, sliderStart.x, sliderStop.x);
		knob.y = sliderStart.y;
	}

	if(selectable) {
		display_SetForeground(KnobColor);
	} else {
		display_SetForeground(Unselectable);
	}
	display_CircleFull(knob.x, knob.y, halfWidth);
	if(selectable) {
		display_SetForeground(Border);
	} else {
		display_SetForeground(BorderUnselectable);
	}
	display_Circle(knob.x, knob.y, halfWidth);
}

void Slider::input(GUIEvent_t* ev) {
	if (!selectable) {
		return;
	}
	switch (ev->type) {
	case EVENT_TOUCH_DRAGGED:
		ev->pos = ev->dragged;
		/* no break */
	case EVENT_TOUCH_PRESSED: {
		bool vertical = size.y > size.x ? true : false;
		int16_t halfWidth = vertical ? size.x / 2 : size.y / 2;
		int32_t newVal;
		if (vertical) {
			if (ev->pos.y < halfWidth) {
				ev->pos.y = halfWidth;
			} else if (ev->pos.y > size.y - halfWidth) {
				ev->pos.y = size.y - halfWidth;
			}
			newVal = util_Map(ev->pos.y, halfWidth, size.y - halfWidth, min,
					max);
		} else {
			if (ev->pos.x < halfWidth) {
				ev->pos.x = halfWidth;
			} else if (ev->pos.x > size.x - halfWidth) {
				ev->pos.x = size.x - halfWidth;
			}
			newVal = util_Map(ev->pos.x, halfWidth, size.x - halfWidth, min,
					max);
		}
		if (newVal != *value) {
			*value = newVal;
			if (cb) {
				cb(cbptr, this);
			}
			requestRedrawFull();
		}
		ev->type = EVENT_NONE;
	}
		break;
	default:
		break;
	}

}
