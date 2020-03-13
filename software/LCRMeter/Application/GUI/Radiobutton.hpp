#pragma once

#include "widget.hpp"
#include "display.h"

class Radiobutton : public Widget {
public:
	using Callback = void (*)(void *ptr, Widget* source, uint8_t value);
	using Set = struct set {
		Radiobutton *first;
		Callback cb;
		void *ptr;
	};
	Radiobutton(uint8_t *value, uint8_t diameter, uint8_t index);

	bool AddToSet(Set &set);
	bool RemoveFromSet();
private:
	void RedrawSet();
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::Radiobutton; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Border = COLOR_FG_DEFAULT;
	static constexpr color_t Unselectable = COLOR_LIGHTGRAY;
	static constexpr color_t Ticked = COLOR(0, 192, 0);
	static constexpr color_t BorderUnselectable = COLOR_GRAY;

    uint8_t *value;
    uint8_t index;
    Radiobutton *nextInSet;
    Set *set;
};

