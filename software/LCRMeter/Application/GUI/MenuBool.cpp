#include "MenuBool.hpp"
#include "Unit.hpp"

//#include "buttons.h"

MenuBool::MenuBool(const char *name, bool *value, void (*cb)(void*, Widget*),
		void *ptr, const char *off_string, const char *on_string) {
	this->value = value;
	strncpy(this->name, name, MaxNameLength);
	this->name[MaxNameLength] = 0;
	this->callback = cb;
	this->cb_ptr = ptr;
	strncpy(off, off_string, MaxStringLength);
	strncpy(on, on_string, MaxStringLength);
	on[MaxStringLength] = 0;
	off[MaxStringLength] = 0;
	selectable = false;
}

void MenuBool::draw(coords_t offset) {
	display_SetFont(*fontName);
	display_SetForeground(Foreground);
	display_SetBackground(Background);
	display_AutoCenterString(name, COORDS(offset.x, offset.y),
			COORDS(offset.x + size.x, offset.y + size.y / 2));
	display_SetFont(*fontValue);
	if (*value) {
		display_SetForeground(ColorOn);
		display_AutoCenterString(on, COORDS(offset.x, offset.y + size.y / 2),
				COORDS(offset.x + size.x, offset.y + size.y));
	} else {
		display_SetForeground(ColorOff);
		display_AutoCenterString(off, COORDS(offset.x, offset.y + size.y / 2),
				COORDS(offset.x + size.x, offset.y + size.y));
	}
}
void MenuBool::input(GUIEvent_t *ev) {
	switch(ev->type) {
	case EVENT_BUTTON_CLICKED:
//		if (!(ev->button & (BUTTON_ENCODER | BUTTON_LEFT | BUTTON_RIGHT))) {
//			// some button was pressed that has no functionality for menu bool
//			break;
//		}
//		/* no break */
	case EVENT_TOUCH_PRESSED:
		*value = !*value;
		requestRedrawFull();
		if (callback)
			callback(cb_ptr, this);
		break;
	}
	ev->type = EVENT_NONE;
}
