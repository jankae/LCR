#ifndef GUI_ENTRY_H_
#define GUI_ENTRY_H_

#include "widget.hpp"
#include "display.h"
#include "font.h"

#include "Unit.hpp"

class Entry : public Widget {
public:
	using Callback = void (*)(void *, Widget*);
	Entry(int32_t *value, const int32_t *max, const int32_t *min, font_t font,
			uint8_t length, const Unit::unit *unit[], const color_t c = COLOR_FG_DEFAULT);
	Entry(int32_t *value, int32_t max, int32_t min, font_t font,
			uint8_t length, const Unit::unit *unit[], const color_t c = COLOR_FG_DEFAULT);
	~Entry();

	void setCallback(Callback cb, void *ptr) {
		this->cb = cb;
		cbptr = ptr;
	}
	void ChangeValue(int32_t *value, const int32_t *max, const int32_t *min,
			const Unit::unit *unit[]) {
		this->value = value;
		this->max = max;
		this->min = min;
		this->unit = unit;
		requestRedraw();
	}

private:
	int32_t constrainValue(int32_t val);
	int32_t InputStringValue(uint32_t multiplier);
	void ValueInputCallback(bool updated);

	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::Entry; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Border = COLOR_FG_DEFAULT;

    int32_t *value;
	bool limitPtr;
    union {
		const int32_t *maxptr;
		int32_t max;
	};
	union {
    	const int32_t *minptr;
		int32_t min;
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

#endif
