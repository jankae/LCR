#include "EventCatcher.hpp"
#include "Unit.hpp"

EventCatcher::EventCatcher(Widget *child, FilterLambda filt, Callback cb,
		void* ptr) {
	firstChild = child;
	firstChild->setPosition(COORDS(0, 0));
	firstChild->setParent(this);
	size = firstChild->getSize();
	selectable = false;
	this->filt = filt;
	this->cb = cb;
	this->ptr = ptr;
	redrawChild = true;
}

EventCatcher::~EventCatcher() {
}

void EventCatcher::drawChildren(coords_t offset) {
	Widget::draw(firstChild, offset);
}

void EventCatcher::input(GUIEvent_t* ev) {
	if (filt(ev)) {
		if (cb) {
			cb(ptr, this, ev);
		}
		ev->type = EVENT_NONE;
	}
}





