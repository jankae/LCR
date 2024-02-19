#include "MenuChooser.hpp"

#include "Dialog/ItemChooserDialog.hpp"

MenuChooser::MenuChooser(const char* name, const char* const * items,
		uint8_t* value, Callback cb, void *ptr, bool popup) {
	/* set member variables */
	this->cb = cb;
	this->ptr = ptr;
	this->items = items;
	this->value = value;
	strncpy(this->name, name, MaxNameLength);
	this->name[MaxNameLength] = 0;
	this->popup = popup;
	selectable = false;
}

void MenuChooser::draw(coords_t offset) {
	display_SetForeground(Foreground);
	display_SetBackground(Background);
	display_AutoCenterString(name, COORDS(offset.x, offset.y + 2),
			COORDS(offset.x + size.x, offset.y + 20));
	display_AutoCenterString(items[*value],
			COORDS(offset.x, offset.y + size.y / 2),
			COORDS(offset.x + size.x, offset.y + size.y));
}

void MenuChooser::input(GUIEvent_t* ev) {
	switch(ev->type) {
//	case EVENT_BUTTON_CLICKED:
//		if (!(ev->button & BUTTON_ENCODER)) {
//			break;
//		}
//		/* no break */
	case EVENT_TOUCH_PRESSED:
		if (popup) {
			// Select new item with itemchooser dialog
			new ItemChooserDialog("Select setting", items, *value,
					[&](void*, bool updated, uint8_t newval) { this->ChooserCallback(updated, newval); }, this);
		} else {
			// simply cycle through items
			uint8_t numItems = 0;
			while(items[numItems]) {
				numItems++;
			}
			(*value)++;
			if (*value >= numItems) {
				*value = 0;
			}
			if (cb) {
				cb(ptr, this);
			}
			requestRedrawFull();
		}
		break;
	}
	ev->type = EVENT_NONE;
}

void MenuChooser::ChooserCallback(bool updated, uint8_t newval) {
	if (updated) {
		*value = newval;
		if (cb) {
			cb(ptr, this);
		}
	}
}
