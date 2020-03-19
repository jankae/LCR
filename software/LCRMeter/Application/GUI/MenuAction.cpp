#include <util.h>
#include "MenuAction.hpp"
#include "Unit.hpp"

//#include "buttons.h"

MenuAction::MenuAction(const char* name, Callback cb, void* ptr) {
	selectable = false;
	this->cb = cb;
	this->ptr = ptr;
	this->name = new char[strlen(name) + 1];
	strcpy(this->name, name);
}

MenuAction::~MenuAction() {
	delete name;
}

void MenuAction::draw(coords_t offset) {
	display_SetForeground(Foreground);
	display_AutoCenterString(name, offset, offset + size);
}



void MenuAction::input(GUIEvent_t* ev) {
	switch(ev->type) {
//	case EVENT_BUTTON_CLICKED:
//		if(!(ev->button &BUTTON_ENCODER)) {
//			break;
//		}
//		/* no break */
	case EVENT_TOUCH_PRESSED:
		if (cb) {
			cb(ptr, this);
		}
		break;
	}
	ev->type = EVENT_NONE;
}
