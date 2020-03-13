#include "keyboard.hpp"

#define LAYOUT_X		10
#define LAYOUT_Y		4

static const char lowerLayout[LAYOUT_Y][LAYOUT_X] = {
		{'1', '2','3','4','5','6','7','8','9','0',},
		{'q', 'w','e','r','t','y','u','i','o','p',},
		{'a', 's','d','f','g','h','j','k','l',';',},
		{'z', 'x','c','v','b','n','m',',','.','-',},
};
static const char upperLayout[LAYOUT_Y][LAYOUT_X] = {
		{'!', '@','#','$','%','^','&','*','(',')',},
		{'Q', 'W','E','R','T','Y','U','I','O','P',},
		{'A', 'S','D','F','G','H','J','K','L',':',},
		{'Z', 'X','C','V','B','N','M','<','>','_',},
};

Keyboard::Keyboard(void (*cb)(char)) {
	keyPressedCallback = cb;
	selectedX = 0;
	selectedY = 0;
	shift = false;

    size.x = LAYOUT_X * spacingX + 1;
    size.y = (LAYOUT_Y + 1) * spacingY + 1;
}

void Keyboard::draw(coords_t offset) {
	/* calculate corners */
    coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x - 1;
	lowerRight.y += size.y - 1;

	/* Draw surrounding rectangle */
//	if (selected) {
//		display_SetForeground(Selected);
//	} else {
		display_SetForeground(Border);
//	}
	display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);
	display_SetForeground(Border);

	/* display dividing lines */
	display_SetForeground(Line);
	uint8_t i;
	for (i = 1; i <= LAYOUT_Y; i++) {
		display_HorizontalLine(upperLeft.x + 1, upperLeft.y + i * spacingX,
				size.x - 2);
	}
	for (i = 1; i < LAYOUT_X; i++) {
		uint8_t length = LAYOUT_Y * spacingY - 1;
		if (i == 3 || i == LAYOUT_X - 3) {
			length += spacingY;
		}
		display_VerticalLine(upperLeft.x + i * spacingX, upperLeft.y + 1,
				length);
	}

	display_SetBackground(Background);
	/* Draw keys */
	for (i = 0; i < LAYOUT_Y; i++) {
		uint8_t j;
		for (j = 0; j < LAYOUT_X; j++) {
			if (i == selectedY && j == selectedX) {
				/* this is the selected key */
				display_SetForeground(Selected);
			} else {
				display_SetForeground(Border);
			}
			char c = lowerLayout[i][j];
			if (shift) {
				c = upperLayout[i][j];
			}
			display_Char(
					upperLeft.x + j * spacingX + (spacingX - Font->width) / 2,
					upperLeft.y + i * spacingY + (spacingY - Font->height) / 2,
					c);
		}
	}

	/* Draw bottom bar with shift, space and backspace */
	/* SHIFT */
	if (selectedY == LAYOUT_Y && selectedX < 3) {
		/* shift is selected */
		display_SetForeground(Selected);
	} else if (shift) {
		/* shift is not selected, but active */
		display_SetForeground(ShiftActive);
	} else {
		display_SetForeground(Border);
	}
	display_String(upperLeft.x + (3 * spacingX - 5 * Font->width) / 2,
			upperLeft.y + LAYOUT_Y * spacingY + (spacingY - Font->height) / 2,
			"SHIFT");

	/* SPACE */
	if (selectedY == LAYOUT_Y && selectedX >= 3 && selectedX < LAYOUT_X - 3) {
		/* space is selected */
		display_SetForeground(Selected);
	} else {
		display_SetForeground(Border);
	}
	display_String(
			upperLeft.x + ((LAYOUT_X - 6) * spacingX - 5 * Font->width) / 2
					+ 3 * spacingX,
			upperLeft.y + LAYOUT_Y * spacingY + (spacingY - Font->height) / 2,
			"SPACE");

	/* DEL */
	if (selectedY == LAYOUT_Y && selectedX >= LAYOUT_X - 3) {
		/* del is selected */
		display_SetForeground (Selected);
	} else {
		display_SetForeground (Border);
	}
	display_String(
			upperLeft.x + (3 * spacingX - 3 * Font->width) / 2
					+ (LAYOUT_X - 3) * spacingX,
			upperLeft.y + LAYOUT_Y * spacingY + (spacingY - Font->height) / 2,
			"DEL");

}

void Keyboard::input(GUIEvent_t *ev) {
	switch(ev->type) {
	case EVENT_ENCODER_MOVED:
		if (ev->movement > 0) {
			/* move to the right, ignore amount of movement */
			selectedX++;
			if (selectedX >= LAYOUT_X) {
				selectedX = 0;
				selectedY++;
				if (selectedY > LAYOUT_Y) {
					selectedY = 0;
				}
			}
		} else {
			/* move to the left, ignore amount of movement */
			if (selectedX > 0) {
				selectedX--;
			} else {
				selectedX = LAYOUT_X - 1;
				if (selectedY > 0) {
					selectedY--;
				} else {
					selectedY = LAYOUT_Y;
				}
			}
		}
		requestRedraw();
		break;
//	case EVENT_BUTTON_CLICKED:
//		if (ev->button & (BUTTON_UNIT1 | BUTTON_ENCODER)) {
//			sendChar();
//		} else if (BUTTON_IS_DIGIT(ev->button)) {
//			/* send this digit directly */
//			if (keyPressedCallback) {
//				keyPressedCallback(BUTTON_TODIGIT(ev->button) + '0');
//			}
//		} else if (BUTTON_IS_ARROW(ev->button)) {
//			switch (ev->button) {
//			case BUTTON_LEFT:
//				if (selectedX > 0) {
//					selectedX--;
//				} else {
//					selectedX = LAYOUT_X - 1;
//				}
//				break;
//			case BUTTON_RIGHT:
//				if (selectedX < LAYOUT_X - 1) {
//					selectedX++;
//				} else {
//					selectedX = 0;
//				}
//				break;
//			case BUTTON_UP:
//				if (selectedY > 0) {
//					selectedY--;
//				} else {
//					selectedY = LAYOUT_Y;
//				}
//				break;
//			case BUTTON_DOWN:
//				if (selectedY < LAYOUT_Y) {
//					selectedY++;
//				} else {
//					selectedY = 0;
//				}
//				break;
//			}
//			requestRedraw();
//		} else if(ev->button == BUTTON_DEL) {
//			if (keyPressedCallback) {
//				keyPressedCallback(0x08);
//			}
//		} else if(ev->button == BUTTON_DOT) {
//			/* send dot directly */
//			if (keyPressedCallback) {
//				keyPressedCallback('.');
//			}
//		} else if(ev->button == BUTTON_SIGN) {
//			/* send sign directly */
//			if (keyPressedCallback) {
//				keyPressedCallback('-');
//			}
//		}
//		break;
	case EVENT_TOUCH_PRESSED:
//		if (selected) {
			/* only react to touch if already selected. This allows
			 * the user to select the widget without already changing the value */
			/* Calculate key pressed */
			selectedX = ev->pos.x / spacingX;
			selectedY = ev->pos.y / spacingY;
			sendChar();
			requestRedraw();
//		}
		break;
	default:
		break;
	}

}

void Keyboard::sendChar() {
	if(selectedY < LAYOUT_Y) {
		/* a character key is selected */
		if (keyPressedCallback) {
			char c = lowerLayout[selectedY][selectedX];
			if (shift) {
				c = upperLayout[selectedY][selectedX];
			}
			keyPressedCallback(c);
		}
	} else {
		if(selectedX <3) {
			/* shift is selected */
			shift = !shift;
			requestRedraw();
		} else if(selectedX >= LAYOUT_X - 3) {
			/* del is selected */
			if (keyPressedCallback) {
				keyPressedCallback(0x08);
			}
		} else {
			/* space is selected */
			if (keyPressedCallback) {
				keyPressedCallback(' ');
			}
		}
	}
}
