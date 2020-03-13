#include "itemChooser.hpp"

#include "Unit.hpp"

ItemChooser::ItemChooser(const char * const *items, uint8_t *value, font_t font,
		uint8_t visibleLines, uint16_t minSizeX) {
	/* set member variables */
	changeCallback = nullptr;
	cbptr = nullptr;
	this->font = font;
	this->itemlist = items;
	this->value = value;
	lines = visibleLines;
	topVisibleEntry = 0;
	/* find number of items and longest item */
	uint8_t maxLength = 0;
	uint8_t numItems;
	for (numItems = 0; itemlist[numItems]; numItems++) {
		uint8_t length = strlen(itemlist[numItems]);
		if (length > maxLength)
			maxLength = length;
	}
	/* calculate size */
	size.y = font.height * visibleLines + 3;
	size.x = font.width * maxLength + 3 + ScrollbarSize;
	if (size.x < minSizeX) {
		size.x = minSizeX;
	}
}

void ItemChooser::draw(coords_t offset) {
	/* Get number of items */
	uint8_t numItems;
	for (numItems = 0; itemlist[numItems]; numItems++)
		;
	/* Constrain selected item (just in case) */
	if (*value >= numItems) {
		*value = numItems - 1;
	}

//	/* Update visible entry offset */
//	if (*value < topVisibleEntry) {
//		topVisibleEntry = *value;
//	} else if (*value >= topVisibleEntry + lines) {
//		topVisibleEntry = *value - lines + 1;
//	}

	/* calculate corners */
	coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x - 1;
	lowerRight.y += size.y - 1;

	/* Draw surrounding rectangle */
//	if (selected) {
//		display_SetForeground(COLOR_SELECTED);
//	} else {
		display_SetForeground(Border);
//	}
	display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
	display_SetForeground (Border);

	/* Display items */
	uint8_t line;
	for (line = 0; line < lines; line++) {
		uint8_t index = line + topVisibleEntry;
		if (index >= numItems) {
			/* item chooser has more lines than entries -> abort */
			break;
		}
		if (index == *value) {
			/* this is the currently selected entry */
			display_SetBackground (Selected);
		} else {
			display_SetBackground(COLOR_BG_DEFAULT);
		}
		display_SetFont(font);
		display_String(upperLeft.x + 1, upperLeft.y + 2 + line * font.height,
				itemlist[index]);
		/* fill rectangle between text and and scrollbar begin with background color */
		uint16_t xbegin = upperLeft.x + 1
				+ font.width * strlen(itemlist[index]);
		uint16_t ybegin = upperLeft.y + 2 + line * font.height;
		uint16_t xstop = lowerRight.x - ScrollbarSize - 1;
		uint16_t ystop = ybegin + font.height - 1;
		display_SetForeground(display_GetBackground());
		display_RectangleFull(xbegin, ybegin, xstop, ystop);
		display_SetForeground(Border);
	}
	/* display scrollbar */
	display_SetForeground(Border);
	display_VerticalLine(lowerRight.x - ScrollbarSize, upperLeft.y, size.y);
	/* calculate beginning and end of scrollbar */
	uint8_t scrollBegin = util_Map(topVisibleEntry, 0, numItems, 0, size.y);
	uint8_t scrollEnd = size.y;
	if (numItems > lines) {
		scrollEnd = util_Map(topVisibleEntry + lines, 0, numItems, 0, size.y);
	}
	/* display position indicator */
	display_SetForeground(ScrollbarColor);
	display_RectangleFull(lowerRight.x - ScrollbarSize + 1,
			upperLeft.y + scrollBegin + 1, lowerRight.x - 1,
			upperLeft.y + scrollEnd - 2);
}
void ItemChooser::input(GUIEvent_t *ev) {
	/* Get number of items */
	uint8_t numItems;
	for (numItems = 0; itemlist[numItems]; numItems++)
		;
	/* Constrain selected item (just in case) */
	if (*value >= numItems) {
		*value = numItems - 1;
	}

	switch (ev->type) {
	case EVENT_ENCODER_MOVED: {
		int16_t newVal = *value + ev->movement;
		if (newVal < 0) {
			newVal = 0;
		} else if (newVal >= numItems) {
			newVal = numItems - 1;
		}
		if (*value != newVal) {
			*value = newVal;
			if (changeCallback) {
				changeCallback(cbptr, this);
			}
			requestRedrawFull();
		}
		ev->type = EVENT_NONE;
	}
		break;
//	case EVENT_BUTTON_CLICKED:
//		if (ev->button == BUTTON_UP && *value > 0) {
//			(*value)--;
//			if (changeCallback) {
//				changeCallback(*this);
//			}
//			requestRedrawFull();
//			ev->type = EVENT_NONE;
//		} else if (ev->button == BUTTON_DOWN && *value < numItems - 1) {
//			(*value)++;
//			if (changeCallback) {
//				changeCallback(*this);
//			}
//			requestRedrawFull();
//			ev->type = EVENT_NONE;
//		}
//		break;
	case EVENT_TOUCH_PRESSED:
		if(ev->pos.x < size.x - ScrollbarSize) {
			int16_t newVal = topVisibleEntry
					+ (ev->pos.y - 2) / font.height;
			if (newVal < 0) {
				newVal = 0;
			} else if (newVal >= numItems) {
				newVal = numItems - 1;
			}
			if (*value != newVal) {
				*value = newVal;
				if (changeCallback) {
					changeCallback(cbptr, this);
				}
				requestRedrawFull();
			}
			ev->type = EVENT_NONE;
		}
		/* no break */
	case EVENT_TOUCH_DRAGGED:
		if(ev->pos.x >= size.x - ScrollbarSize) {
			/* Get number of items */
			uint8_t numItems;
			for (numItems = 0; itemlist[numItems]; numItems++)
				;
			int16_t scrollBarLength = size.y * lines / numItems;
			/* adjust horizontal canvas offset */
			if (ev->type == EVENT_TOUCH_DRAGGED) {
				ev->pos.y = ev->dragged.y;
			}
			int16_t newOffset = util_Map(ev->pos.y, scrollBarLength / 2,
					size.y - scrollBarLength / 2, 0,
					numItems - lines);
			newOffset = constrain_int16_t(newOffset, 0, numItems - lines);
			if(newOffset != topVisibleEntry) {
				topVisibleEntry = newOffset;
				requestRedrawFull();
			}
			ev->type = EVENT_NONE;
		}
		break;
	default:
		break;
	}
	return;
}
