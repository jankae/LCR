#ifndef GUI_BUTTON_H_
#define GUI_BUTTON_H_

#include "widget.hpp"
#include "display.h"
#include "font.h"

#include "Unit.hpp"

class Button : public Widget {
public:

	Button(const char *name, font_t font, Callback cb, void *ptr,
			coords_t minSize = COORDS(0, 0));
	~Button();

	char *getName() {
		return name;
	}
private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::Button; };

	static constexpr color_t Foreground = COLOR_BLACK;
	static constexpr color_t Background = COLOR_BG_DEFAULT;

    Callback cb;
    void *ptr;
    char *name;
    font_t font;
    coords_t fontStart;
    bool pressed;
};

#endif
