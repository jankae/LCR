/*
 * EventCatcher.hpp
 *
 *  Created on: Jun 12, 2019
 *      Author: jan
 */

#ifndef EVENTCATCHER_HPP_
#define EVENTCATCHER_HPP_

#include <util.h>
#include <widget.hpp>
#include "display.h"
#include "font.h"

class EventCatcher : public Widget {
public:
	using Callback = void(*)(void *ptr, Widget *source, GUIEvent_t *ev);
	using FilterLambda = bool (*)(GUIEvent_t * const ev);

	EventCatcher(Widget *child, FilterLambda filt, Callback cb, void *ptr);
	~EventCatcher();

private:
	void drawChildren(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::EventCatcher; };

	FilterLambda filt;
    Callback cb;
    void *ptr;
};



#endif /* EVENTCATCHER_HPP_ */
