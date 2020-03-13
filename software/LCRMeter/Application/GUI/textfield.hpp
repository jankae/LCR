#ifndef GUI_TEXTFIELD_H_
#define GUI_TEXTFIELD_H_

#include "widget.hpp"
#include "display.h"
#include "font.h"

class Textfield : public Widget {
public:
	Textfield(const char *text, const font_t font);
	~Textfield();

private:
	void draw(coords_t offset) override;

	Widget::Type getType() override { return Widget::Type::Textfield; };

	static constexpr color_t Foreground = COLOR_FG_DEFAULT;
	static constexpr color_t Background = COLOR_BG_DEFAULT;

    char *text;
    font_t font;
};

#endif
