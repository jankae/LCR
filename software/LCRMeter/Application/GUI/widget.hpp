#ifndef GUI_WIDGET_H_
#define GUI_WIDGET_H_

#include <string.h>
#include <stdint.h>

#include "util.h"
#include "events.hpp"

class Widget {
	/* Classes which can have children need extended access to their childrens members */
	friend class Container;
	friend class Window;
public:
	enum class Type : uint8_t {
		Button,
		Checkbox,
		Container,
		Custom,
		Entry,
		Graph,
		ItemChooser,
		Keyboard,
		Label,
		Progressbar,
		Sevensegment,
		Textfield,
		Widget,
		Window,
		Scopescreen,
		Desktop,
		Radiobutton,
		Slider,
	};

	Widget();
	virtual ~Widget();

	using Callback = void (*)(void*, Widget*);

	static void draw(Widget *w, coords_t pos);
	static void input(Widget *w, GUIEvent_t *ev);

	static Widget *getSelected() {
		return selectedWidget;
	}

//	static void deselect();
//	void select(bool down = true);

	void requestRedrawChildren();
	void requestRedraw();
	void requestRedrawFull();

	void setSelectable(bool s) {
		if(s && !selectable) {
			selectable = true;
			requestRedrawFull();
		} else if(!s && selectable) {
			selectable = false;
			requestRedrawFull();
		}
	}

	void setVisible(bool v) {
		if(visible != v) {
			visible = v;
			requestRedrawFull();
		}
	}

	bool isInArea(coords_t pos);

	coords_t getSize() {
		return size;
	}

	void setPosition(coords_t pos) {
		position = pos;
	}

	void addChild(Widget *w, coords_t pos);

protected:
//	Widget* IntSelectChild();

	virtual void draw(coords_t offset) { return; }
	virtual void input(GUIEvent_t *ev) { return; }
	virtual void drawChildren(coords_t offset) { return; }
	virtual Type getType() = 0;

	static Widget *selectedWidget;

	Widget *parent;
	Widget *firstChild;
	Widget *next;

	coords_t position;
	coords_t size;
	bool visible :1;
//	bool selected :1;
	bool selectable :1;
//	bool focus :1;
	/* this widget needs to be redrawn */
	bool redraw :1;
	/* the widget area has to be cleared and the widget redrawn completely */
	bool redrawClear :1;
	/* some widget down this widgets branch has to be redrawn */
	bool redrawChild :1;
};

#endif
