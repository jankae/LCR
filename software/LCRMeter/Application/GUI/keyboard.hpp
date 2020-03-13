#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "widget.hpp"
#include "display.h"

class Keyboard : public Widget {
public:
	Keyboard(void (*cb)(char));

private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::Keyboard; };

	void sendChar(void);

	static constexpr color_t Border = COLOR_FG_DEFAULT;
	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Line = COLOR_GRAY;
	static constexpr color_t Selected = COLOR_SELECTED;
	static constexpr color_t ShiftActive = COLOR_DARKGREEN;

	static constexpr font_t const *Font = &Font_Big;
	static constexpr uint8_t spacingX = 31;
	static constexpr uint8_t spacingY = 30;

    void (*keyPressedCallback)(char);
    uint8_t selectedX, selectedY;
    bool shift;
};

#endif
