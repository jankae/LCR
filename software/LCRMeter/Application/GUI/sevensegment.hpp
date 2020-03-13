#ifndef GUI_SEVEN_H_
#define GUI_SEVEN_H_

#include "widget.hpp"
#include "display.h"
//#include "font.h"


#include "Unit.hpp"
//#include "dialog.h"

class SevenSegment : public Widget {
public:
	SevenSegment(int32_t *value, uint8_t sLength, uint8_t sWidth, uint8_t length, uint8_t dot, color_t color);

private:
	void draw_Digit(int16_t x, int16_t y, uint8_t digit);

	void draw(coords_t offset) override;

	Widget::Type getType() override { return Widget::Type::Sevensegment; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;

    int32_t *value;
    uint8_t segmentLength;
    uint8_t segmentWidth;
    uint8_t length;
    uint8_t dot;
    color_t color;
};

#endif
