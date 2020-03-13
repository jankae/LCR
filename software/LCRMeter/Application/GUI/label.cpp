#include "label.hpp"

Label::Label(uint8_t length, font_t font, Orientation o) {
	color = Foreground;
	this->font = font;
	this->orient = o;
	text = nullptr;
	this->selectable = false;
	/* calculate size */
	size.y = font.height;
	size.x = font.width * length;
	fontStartX = 0;
}

Label::Label(const char *text, font_t font) {
	color = Foreground;
	this->font = font;
	this->orient = orient;
	this->text = nullptr;
	this->selectable = false;
	/* calculate size */
	size.y = font.height;
	size.x = font.width * strlen(text);
	fontStartX = 0;
	setText(text);
}

Label::~Label() {
	if(text) {
		delete text;
	}
}

void Label::setText(const char *text) {
	/* Free previous label text */
	if(this->text) {
		delete this->text;
	}
	uint16_t namelength = strlen(text);
	if(namelength > size.x / font.width) {
		namelength = size.x / font.width;
	}
	this->text = new char[namelength + 1];
	memcpy(this->text, text, namelength + 1);

	switch (orient) {
	case Orientation::LEFT:
		/* always starting right at the label */
		fontStartX = 0;
		break;
	case Orientation::RIGHT:
		/* orient font on right side of label */
		fontStartX = size.x - namelength * font.width;
		break;
	case Orientation::CENTER:
		/* orient font in the middle */
		fontStartX = (size.x - namelength * font.width) / 2;
		break;
	}
	/* text has changed, this widget has to be drawn again */
	requestRedrawFull();
}

void Label::draw(coords_t offset) {
    display_SetForeground(color);
    display_SetBackground(Background);
	display_SetFont(font);
	if (text) {
		display_String(offset.x + fontStartX, offset.y, text);
	}
}
