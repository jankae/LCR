#ifndef GUI_CONTAINER_H_
#define GUI_CONTAINER_H_

#include <util.h>
#include "widget.hpp"
#include "display.h"

class Container : public Widget {
public:
	Container(coords_t size);

	void attach(Widget *w, coords_t offset);
private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;
	void drawChildren(coords_t offset) override;

	Widget::Type getType() override { return Widget::Type::Container; };

	static constexpr uint8_t ScrollbarSize = 20;
	static constexpr color_t ScrollbarColor = COLOR_ORANGE;
	static constexpr color_t LineColor = COLOR_FG_DEFAULT;

	coords_t canvasSize;
	coords_t viewingSize;
	coords_t canvasOffset;
	coords_t scrollBarLength;
	bool editing;
	bool focussed;
	bool scrollVertical;
	bool scrollHorizontal;
};

#endif
