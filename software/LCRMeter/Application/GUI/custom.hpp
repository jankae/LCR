#ifndef CUSTOM_H_
#define CUSTOM_H_

#include "widget.hpp"

class Custom: public Widget {
public:
	Custom(coords_t size, void (*draw)(Widget&, coords_t),
			void (*input)(Widget&, GUIEvent_t *)) {
		this->size = size;
		drawCB = draw;
		inputCB = input;
	}

private:
	void draw(coords_t offset) override {
		if (drawCB) {
			drawCB(*this, offset);
		}
	}

	void input(GUIEvent_t *ev) override {
		if (inputCB) {
			inputCB(*this, ev);
		}
	}

	Widget::Type getType() override { return Widget::Type::Custom; };

	void (*drawCB)(Widget&, coords_t);
	void (*inputCB)(Widget&, GUIEvent_t*);
};

#endif
