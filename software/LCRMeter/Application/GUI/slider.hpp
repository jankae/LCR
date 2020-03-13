#pragma once

#include "widget.hpp"
#include "display.h"

#include "Unit.hpp"

class Slider : public Widget {
public:
	using Callback = void (*)(void *, Widget*);
	Slider(int32_t *value, int32_t min, int32_t max, coords_t size);
	~Slider();

	void setCallback(Callback cb, void *ptr) {
		this->cb = cb;
		cbptr = ptr;
	}

private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::Slider; };

	static constexpr color_t Border = COLOR_BLACK;
	static constexpr color_t KnobColor = COLOR_ORANGE;
	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Unselectable = COLOR_LIGHTGRAY;
	static constexpr color_t BorderUnselectable = COLOR_GRAY;

    int32_t *value;
    int32_t min;
    int32_t max;
	Callback cb;
	void *cbptr;
};

