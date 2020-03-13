#include "entry.hpp"

#include "ValueInput.hpp"
#include "cast.hpp"

Entry::Entry(int32_t *value, const int32_t *max, const int32_t *min, font_t font,
		uint8_t length, const Unit::unit *unit[], const color_t c) {
	/* set member variables */
	this->value = value;
	limitPtr = true;
	this->maxptr = max;
	this->minptr = min;
	this->font = font;
	this->unit = unit;
	this->length = length;
    cb = nullptr;
    cbptr = nullptr;
    editing = false;
    dotSet = false;
    editPos = 0;
	size.y = font.height + 3;
	size.x = font.width * length + 3;
	inputString = new char[length + 1];
	color = c;
}

Entry::Entry(int32_t* value, int32_t max, int32_t min, font_t font,
		uint8_t length, const Unit::unit* unit[], const color_t c) {
	/* set member variables */
	this->value = value;
	limitPtr = false;
	this->max = max;
	this->min = min;
	this->font = font;
	this->unit = unit;
	this->length = length;
    cb = nullptr;
    cbptr = nullptr;
    editing = false;
    dotSet = false;
    editPos = 0;
	size.y = font.height + 3;
	size.x = font.width * length + 3;
	inputString = new char[length + 1];
	color = c;
}

Entry::~Entry() {
	if(inputString) {
		delete inputString;
	}
}

int32_t Entry::constrainValue(int32_t val) {
	int32_t high = max;
	int32_t low = min;
	if (limitPtr) {
		high = maxptr ? *maxptr : INT32_MAX;
		low = minptr ? *minptr : INT32_MIN;
	}
	if (val > high) {
		return high;
	} else if (val < low) {
		return low;
	}
	return val;
}

int32_t Entry::InputStringValue(uint32_t multiplier) {
    int64_t value = 0;
    uint8_t i;
    uint32_t div = 0;
    for (i = 0; i < length; i++) {
        if (inputString[i] >= '0' && inputString[i] <= '9') {
            value *= 10;
            value += inputString[i] - '0';
            if (div) {
                div *= 10;
            }
        } else if (inputString[i] == '.') {
            div = 1;
        }
    }
    value *= multiplier;
    if (div) {
        value /= div;
    }
	if (inputString[0] == '-') {
		value = -value;
	}
    return value;
}

void Entry::draw(coords_t offset) {
    /* calculate corners */
    coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x - 1;
	lowerRight.y += size.y - 1;
//	if (selected) {
//		display_SetForeground(COLOR_SELECTED);
//	} else {
		display_SetForeground(Border);
//	}
	display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);

	/* display string */
	if (!editing) {
		/* construct value string */
		Unit::StringFromValue(inputString, length, *value, unit);
		if (selectable) {
			display_SetForeground(color);
		} else {
			display_SetForeground(COLOR_GRAY);
		}
	} else {
		display_SetForeground(COLOR_SELECTED);
	}
	if (selectable) {
		display_SetBackground(Background);
	} else {
		display_SetBackground(COLOR_UNSELECTABLE);
	}
	display_SetFont(font);
	display_String(upperLeft.x + 1, upperLeft.y + 2, inputString);

}

void Entry::ValueInputCallback(bool updated) {
	if(updated) {
		*value = constrainValue(*value);
		if(cb) {
			cb(cbptr, this);
		}
	}
}

void Entry::input(GUIEvent_t *ev) {
	if (!selectable) {
		return;
	}
    switch(ev->type) {
    case EVENT_TOUCH_RELEASED:
		new ValueInput("New value?", value, unit,
				pmf_cast<void (*)(void*, bool), Entry,
						&Entry::ValueInputCallback>::cfn, this);
		ev->type = EVENT_NONE;
    	break;
    default:
    	break;
    }
    return;
}


