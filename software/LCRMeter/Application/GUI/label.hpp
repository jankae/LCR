#ifndef GUI_LABEL_H_
#define GUI_LABEL_H_

#include "widget.hpp"
#include "display.h"
#include "font.h"

class Label : public Widget {
public:
	enum class Orientation {
		LEFT, CENTER, RIGHT
	};
	Label(uint8_t length, font_t font, Orientation o);
	Label(const char *text, font_t font);
	~Label();

	void setText(const char *text);
	void setColor(color_t c) {
		color = c;
		requestRedraw();
	}

private:
	void draw(coords_t offset) override;

	Widget::Type getType() override { return Widget::Type::Label; };

	static constexpr color_t Foreground = COLOR_BLACK;
	static constexpr color_t Background = COLOR_BG_DEFAULT;

    char *text;
    Orientation orient;
    font_t font;
    uint16_t fontStartX;
    color_t color;
};

#endif
