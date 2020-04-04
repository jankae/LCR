#pragma once

#include "widget.hpp"
#include "display.h"
#include "font.h"
#include <limits>
#include "Unit.hpp"
#include "Dialog/ValueInput.hpp"

template<typename T>
class Entry : public Widget {
public:
	using Callback = void (*)(void *, Widget*);
	Entry(T *value, const T *max, const T *min, font_t font, uint8_t length,
			const Unit::unit *unit[] = Unit::None, const color_t c = COLOR_FG_DEFAULT) {
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
	Entry(T *value, T max, T min, font_t font, uint8_t length,
			const Unit::unit *unit[] = Unit::None, const color_t c = COLOR_FG_DEFAULT) {
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
	~Entry() {
		if(inputString) {
			delete inputString;
		}
	}

	void setCallback(Callback cb, void *ptr) {
		this->cb = cb;
		cbptr = ptr;
	}
private:
	T constrainValue(T val) {
		T high = max;
		T low = min;
		if (limitPtr) {
			high = maxptr ? *maxptr : std::numeric_limits<T>::max();
			low = minptr ? *minptr : std::numeric_limits<T>::lowest();
		}
		if (val > high) {
			return high;
		} else if (val < low) {
			return low;
		}
		return val;
	}
	void ValueInputCallback(bool updated) {
		if(updated) {
			*value = constrainValue(*value);
			if(cb) {
				cb(cbptr, this);
			}
		}
	}

	void CreateString() {
		Unit::StringFromValue(inputString, length, *value, unit);
	}

	void draw(coords_t offset) override {
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
			CreateString();
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
	void input(GUIEvent_t *ev) override {
		if (!selectable) {
			return;
		}
	    switch(ev->type) {
	    case EVENT_TOUCH_RELEASED:
			new ValueInput<T>("New value?", value, unit,
					pmf_cast<void (*)(void*, bool), Entry,
							&Entry::ValueInputCallback>::cfn, this);
			ev->type = EVENT_NONE;
	    	break;
	    default:
	    	break;
	    }
	    return;
	}

	Widget::Type getType() override { return Widget::Type::Entry; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Border = COLOR_FG_DEFAULT;

    T *value;
	bool limitPtr;
    union {
		const T *maxptr;
		T max;
	};
	union {
    	const T *minptr;
		T min;
	};
    font_t font;
    const Unit::unit **unit;
    uint8_t length;
    color_t color;
	bool editing;
	bool dotSet;
	Callback cb;
	void *cbptr;
	uint8_t editPos;
    char *inputString;
};

template <>
void Entry<float>::CreateString();


