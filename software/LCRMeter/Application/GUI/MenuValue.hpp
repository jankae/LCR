#pragma once
#include <limits>

#include "menuentry.hpp"
#include "Unit.hpp"
#include "Dialog/ValueInput.hpp"
#include "cast.hpp"
//#include "buttons.h"

template<typename T>
class MenuValue: public MenuEntry {
public:
	MenuValue(const char *name, T *value, const Unit::unit *unit[],
			Callback cb = nullptr, void *ptr = nullptr, T min =
					std::numeric_limits<T>::min(), T max =
					std::numeric_limits<T>::max()) {
		/* set member variables */
		this->cb = cb;
		this->ptr = ptr;
		this->unit = unit;
		this->value = value;
		this->min = min;
		this->max = max;
		strncpy(this->name, name, MaxNameLength);
		this->name[MaxNameLength] = 0;
		selectable = false;
	}

private:
	void CreateUnitString(char *s, uint8_t len) {
		Unit::StringFromValue(s, len, *value, unit);
	}
	void draw(coords_t offset) override {
		display_SetForeground(Foreground);
		display_SetBackground(Background);
		display_AutoCenterString(name, COORDS(offset.x, offset.y),
				COORDS(offset.x + size.x, offset.y + size.y / 2));
		uint8_t len = size.x / fontValue->width;
		char s[len + 1];
		CreateUnitString(s, len);
		display_AutoCenterString(s, COORDS(offset.x, offset.y + size.y / 2),
				COORDS(offset.x + size.x, offset.y + size.y));
	}
	void input(GUIEvent_t *ev) override {
		char firstChar = 0;
		switch(ev->type) {
//		case EVENT_BUTTON_CLICKED:
//			if(BUTTON_IS_DIGIT(ev->button)) {
//				firstChar = BUTTON_TODIGIT(ev->button) + '0';
//			} else if(ev->button & BUTTON_DOT) {
//				firstChar = '.';
//			} else if(ev->button & BUTTON_SIGN) {
//				firstChar = '-';
//			} else if(!(ev->button & (BUTTON_ENCODER))) {
//				break;
//			}
//			/* no break */
		case EVENT_TOUCH_PRESSED:
			new ValueInput<T>("New value:", value, unit,
					pmf_cast<void (*)(void*, bool), MenuValue,
							&MenuValue::ValueCallback>::cfn, this, firstChar);
		}
		ev->type = EVENT_NONE;
	}
	void ValueCallback(bool updated) {
		if (updated) {
			if (*value > max) {
				*value = max;
			} else if (*value < min) {
				*value = min;
			}
			if (cb) {
				cb(ptr, this);
			}
		}
	}

	Widget::Type getType() override { return Widget::Type::MenuValue; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Foreground = COLOR_FG_DEFAULT;
	static constexpr uint8_t MaxNameLength = 25;
	static constexpr const font_t *fontValue = &Font_Medium;

	Callback cb;
	void *ptr;
	T *value;
	T min, max;
    char name[MaxNameLength + 1];
    const Unit::unit **unit;
};

template <> void MenuValue<float>::CreateUnitString(char *s, uint8_t len);
