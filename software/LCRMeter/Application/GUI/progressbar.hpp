#ifndef PROGRESSBAR_H_
#define PROGRESSBAR_H_

#include "widget.hpp"
#include "display.h"

#include "Unit.hpp"

class ProgressBar : public Widget {
public:
	ProgressBar(coords_t size, color_t barColor = Bar);

	void setState(uint8_t state);
private:
	void draw(coords_t offset) override;

	Widget::Type getType() override { return Widget::Type::Progressbar; };

	static constexpr color_t Border = COLOR_BLACK;
	static constexpr color_t Bar = COLOR_GREEN;
	static constexpr color_t Background = COLOR_BG_DEFAULT;

    uint8_t state;
    color_t barColor;
};

#endif
