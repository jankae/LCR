#include "ValueInput.hpp"

template <> void ValueInput<float>::CreateUnitButtons(Container *c, coords_t topLeft, uint16_t incX,
		uint16_t incY, uint16_t sizeX, uint16_t sizeY) {
	uint8_t prefix = 1; // start with pico prefix
	for (uint8_t i = 0; i < 2; i++) {
		for (uint8_t j = 0; j < 3; j++) {
			char name[3];
			if (Unit::SI_prefixes[prefix].name != ' ') {
				name[0] = Unit::SI_prefixes[prefix].name;
				name[1] = 0;
			} else {
				strcpy(name, "OK");
			}
			auto button = new Button(name, Font_Big,
					[&](void*, Widget *w) { this->UnitPressed(w); }, this,
					SIZE(sizeX, sizeY));
			c->attach(button, topLeft + COORDS(incX * i, incY * j));
			prefix++;
		}
	}
}

template <> void ValueInput<float>::UnitPressed(Widget *w) {
	float val = (float) Unit::ValueFromString(string, 100000) / 100000.0f;
	Button *b = (Button*) w;
	if (strcmp(b->getName(), "OK")) {
		for (uint8_t i = 0; i < ARRAY_SIZE(Unit::SI_prefixes); i++) {
			if (b->getName()[0] == Unit::SI_prefixes[i].name) {
				val *= Unit::SI_prefixes[i].factor;
			}
		}
	} else {
		// "OK"-Button pressed ->scaling factor = 1, nothing to do
	}
	*value = val;
	auto cb_buf = cb;
	auto ptr_buf = ptr;
	delete this;
	if (cb_buf) {
		cb_buf(ptr_buf, true);
	}
}
