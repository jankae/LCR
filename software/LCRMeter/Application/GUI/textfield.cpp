#include "textfield.hpp"

Textfield::Textfield(const char *text, const font_t font) {
    this->font = font;

	/* extract necessary size from text */
	uint16_t maxWidth = 0;
	uint16_t height = font.height;
	uint16_t width = 0;
	const char *s = text;
	while (*text) {
		if (*text == '\n') {
			height += font.height;
			width = 0;
		} else {
			width += font.width;
			if (width > maxWidth) {
				maxWidth = width;
			}
		}
		text++;
	}
	/* set widget size */
	size.x = maxWidth;
	size.y = height;

	selectable = false;
	/* copy text */
	this->text = new char[strlen(s) + 1];
	memcpy(this->text, s, strlen(s) + 1);
}

Textfield::~Textfield() {
	if(text) {
		delete text;
	}
}

void Textfield::draw(coords_t offset) {
	const char *s = text;
	coords_t pos = offset;
	display_SetForeground(Foreground);
	display_SetBackground(Background);
	display_SetFont(font);
	while (*s) {
		if (*s == '\n') {
			pos.x = offset.x;
			pos.y += font.height;
		} else {
			display_Char(pos.x, pos.y, *s);
			pos.x += font.width;
		}
		s++;
	}
}
