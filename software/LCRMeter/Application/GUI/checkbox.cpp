#include "checkbox.hpp"

Checkbox::Checkbox(bool *value, Callback cb, void *ptr, coords_t size) {
    this->value = value;
    this->cb = cb;
    cbptr = ptr;
    this->size = size;
}

void Checkbox::draw(coords_t offset) {
	/* calculate corners */
	coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x - 1;
	lowerRight.y += size.y - 1;
	if (selectable) {
		display_SetForeground(Border);
	} else {
		display_SetForeground (Unselectable);
	}
	display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
	if (*value) {
		if (selectable) {
			display_SetForeground(Ticked);
		} else {
			display_SetForeground (Unselectable);
		}
		display_Line(upperLeft.x + 2, lowerRight.y - size.y / 3,
				upperLeft.x + size.x / 3, lowerRight.y - 2);
		display_Line(upperLeft.x + 2, lowerRight.y - size.y / 3 - 1,
				upperLeft.x + size.x / 3 + 1, lowerRight.y - 2);
		display_Line(upperLeft.x + 2, lowerRight.y - size.y / 3 - 2,
				upperLeft.x + size.x / 3 + 2, lowerRight.y - 2);
		display_Line(lowerRight.x - 2, upperLeft.y + 2,
				upperLeft.x + size.x / 3, lowerRight.y - 2);
		display_Line(lowerRight.x - 2, upperLeft.y + 3,
				upperLeft.x + size.x / 3 + 1, lowerRight.y - 2);
		display_Line(lowerRight.x - 2, upperLeft.y + 4,
				upperLeft.x + size.x / 3 + 2, lowerRight.y - 2);
	} else {
		if (selectable) {
			display_SetForeground(Unticked);
		} else {
			display_SetForeground (Unselectable);
		}
		display_Line(upperLeft.x + 3, upperLeft.y + 3, lowerRight.x - 3,
				lowerRight.y - 3);
		display_Line(upperLeft.x + 4, upperLeft.y + 3, lowerRight.x - 3,
				lowerRight.y - 4);
		display_Line(upperLeft.x + 3, upperLeft.y + 4, lowerRight.x - 4,
				lowerRight.y - 3);
		display_Line(upperLeft.x + 3, lowerRight.y - 3, lowerRight.x - 3,
				upperLeft.y + 3);
		display_Line(upperLeft.x + 4, lowerRight.y - 3, lowerRight.x - 3,
				upperLeft.y + 4);
		display_Line(upperLeft.x + 3, lowerRight.y - 4, lowerRight.x - 4,
				upperLeft.y + 3);
	}
}

void Checkbox::input(GUIEvent_t *ev) {
	switch(ev->type) {
	case EVENT_TOUCH_RELEASED:
		if (selectable) {
			*value = !*value;
			requestRedrawFull();
			if (cb)
				cb(cbptr, this);
			ev->type = EVENT_NONE;
		}
		break;
	default:
		break;
	}
	return;
}
