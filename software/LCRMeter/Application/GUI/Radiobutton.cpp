#include "Radiobutton.hpp"

Radiobutton::Radiobutton(uint8_t *value, uint8_t diameter, uint8_t index) {
	this->value = value;
	size.x = diameter;
	size.y = diameter;
	this->index = index;
	nextInSet = nullptr;
	set = nullptr;
}

bool Radiobutton::AddToSet(Set &set) {
	if(this->set) {
		// button already associated with a set
		return false;
	}
	if(!set.first) {
		set.first = this;
	} else {
		Radiobutton *last = set.first;
		while(last->nextInSet) {
			last = last->nextInSet;
		}
		last->nextInSet = this;
	}
	this->set = &set;
	return true;
}

bool Radiobutton::RemoveFromSet() {
	if(!set) {
		return false;
	} else {
		if(set->first == this) {
			// this is the very first entry
			set->first = nextInSet;
		} else {
			Radiobutton *prev = set->first;
			while(prev->nextInSet != this) {
				prev = prev->nextInSet;
				if(!prev) {
					// reached end of set, this was not in it
					return false;
				}
			}
			prev->nextInSet = nextInSet;
		}
	}
	nextInSet = nullptr;
	set = nullptr;
	return true;
}

void Radiobutton::draw(coords_t offset) {
	/* calculate corners */
	coords_t center;
	center.x = offset.x + size.x / 2;
	center.y = offset.y + size.y / 2;
	if (!selectable) {
		display_SetForeground(Unselectable);
		display_CircleFull(center.x, center.y, size.x / 2);
	}
	if (*value == index) {
		if (selectable) {
			display_SetForeground(Ticked);
		} else {
			display_SetForeground(BorderUnselectable);
		}
		display_CircleFull(center.x, center.y, size.x / 3);
	}
	if (selectable) {
		display_SetForeground(Border);
	} else {
		display_SetForeground(BorderUnselectable);
	}
	display_Circle(center.x, center.y, size.x / 2);
}

void Radiobutton::RedrawSet() {
	if (set) {
		Radiobutton *b = set->first;
		while (b) {
			b->requestRedrawFull();
			b = b->nextInSet;
		}
		if (set->cb) {
			set->cb(set->ptr, this, index);
		}
	}
}

void Radiobutton::input(GUIEvent_t *ev) {
	switch(ev->type) {
	case EVENT_TOUCH_RELEASED:
		if (selectable) {
			if (*value != index) {
				*value = index;
				RedrawSet();
			}
			ev->type = EVENT_NONE;
		}
		break;
	default:
		break;
	}
	return;
}
