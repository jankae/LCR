#include "window.hpp"
#include "log.h"
#include "gui.hpp"

extern Widget *topWidget;
extern bool isPopup;

Window::Window(const char *title, font_t font, coords_t size) {
	this->size = size;
	/* center window on screen */
	position.x = (DISPLAY_WIDTH - size.x) / 2;
	position.y = (DISPLAY_HEIGHT - size.y) / 2;
	/* a new widget must have its area cleared */
	redrawClear = 1;
	this->font = font;
	/* set title */
	uint16_t titlelength = strlen(title);
	this->title = new char[titlelength + 1];
	memcpy(this->title, title, titlelength + 1);
	/* store last top widget (will be restored when this window closes) */
	lastTopWidget = topWidget;
	topWidget = this;
	lastPopup = isPopup;
	isPopup = true;
}
Window::~Window() {
	/* restore last top widget */
//	Widget::deselect();
	topWidget = lastTopWidget;
	isPopup = lastPopup;
	/* request full redraw of new top widget */
	topWidget->requestRedrawFull();
//	// TODO this is extremely ugly
//	GUIEvent_t ev;
//	ev.type = EVENT_WINDOW_CLOSE;
//	ev.w = this;
//	GUI::SendEvent(&ev);

	if (title) {
		delete title;
	}
}

void Window::setMainWidget(Widget *w) {
	if(firstChild) {
		/* window already has a widget in it */
		LOG(Log_GUI, LevelCrit, "Window already has a widget");
	}
	coords_t maxSize = getAvailableArea();
	if(w->getSize().x > maxSize.x || w->getSize().y > maxSize.y) {
		/* widget doesn't fit in window */
		/* This potentially allows for a memory leak if the application doesn't check for
		 * a return code. The widget (which failed to be attached to the window) won't get freed
		 * when the window is closed.
		 * Workaround: As an empty window is not useful at all, this will only happen during
		 * a software error -> display error message */
		LOG(Log_GUI, LevelCrit, "Widget too big for window");
	}
	firstChild = w;
	redrawChild = 1;
	w->parent = this;
	/* set child offset */
	w->setPosition(COORDS(1, font.height + 4));
}
coords_t Window::getAvailableArea() {
	return SIZE(size.x - 2, size.y - font.height - 5);
}

void Window::draw(coords_t offset) {
	/* calculate corners */
	coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x - 1;
	lowerRight.y += size.y - 1;
	display_SetForeground(Border);
	/* draw outline */
	display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
	/* draw dividing line under the title bar */
	display_HorizontalLine(upperLeft.x + 1, upperLeft.y + font.height + 3,
			size.x - 2);
	/* draw dividing line between title and close button */
//	display_VerticalLine(upperLeft.x + font.height + 3, upperLeft.y + 1,
//			font.height + 2);
//	/* fill close area with background color */
//	display_SetForeground(WINDOW_CLOSE_AREA_COLOR);
//	display_RectangleFull(upperLeft.x + 1, upperLeft.y + 1,
//			upperLeft.x + font.height + 2,
//			upperLeft.y + font.height + 2);
//	/* draw X in close area */
//	display_SetForeground(WINDOW_CLOSE_X_COLOR);
//	display_Line(upperLeft.x + 2, upperLeft.y + 2,
//			upperLeft.x + font.height + 1,
//			upperLeft.y + font.height + 1);
//	display_Line(upperLeft.x + 2, upperLeft.y + font.height + 1,
//			upperLeft.x + font.height + 1, upperLeft.y + 2);
	/* fill title bar with background color */
	display_SetForeground(TitleBackground);
	display_RectangleFull(upperLeft.x + 1/*+ font.height + 4*/, upperLeft.y + 1,
			lowerRight.x - 1, upperLeft.y + font.height + 2);
	/* add title */
	display_SetForeground(TitleForeground);
	display_SetBackground(TitleBackground);
	display_SetFont(font);
	display_String(upperLeft.x + 1/*+ font.height + 5*/, upperLeft.y + 2,
			title);
}

//void Window::input(Event *ev) {
//	switch (ev->type) {
//	case EVENT_TOUCH_RELEASED:
//		if (ev->pos.y <= font.height + 3) {
//			/* mark event as handled */
//			ev->type = EVENT_NONE;
//			if (ev->pos.x <= font.height + 3) {
//				/* clicked into window close area, close this window */
//				Event ev = { .type = EVENT_WINDOW_CLOSE, .w = w };
//				gui_SendEvent(&ev);
//			}
//		}
//		break;
//	default:
//		break;
//	}
//}

void Window::drawChildren(coords_t offset) {
	if (firstChild) {
		Widget::draw(firstChild, offset);
	}
}
