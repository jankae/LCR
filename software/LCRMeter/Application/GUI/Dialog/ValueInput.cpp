#include "ValueInput.hpp"

#include "cast.hpp"
#include "log.h"

ValueInput::ValueInput(const char* title, int32_t* value,
		const Unit::unit *unit[], Callback cb, void* ptr) {
	LOG(Log_GUI, LevelInfo, "Creating value dialog, free heap: %lu",
			xPortGetFreeHeapSize());
	this->cb = cb;
	this->ptr = ptr;
	this->unit = unit;
	this->value = value;
	inputIndex = 0;
	memset(string, 0, sizeof(string));
	w = new Window(title, Font_Big, COORDS(320, 240));
	auto c = new Container(w->getAvailableArea());
	constexpr uint16_t fieldOffsetX = 5;
	constexpr uint16_t fieldOffsetY = 30;
	constexpr uint16_t fieldIncX = 52;
	constexpr uint16_t fieldIncY = 50;
	constexpr uint16_t fieldSizeX = 47;
	constexpr uint16_t fieldSizeY = 45;
	l = new Label((fieldSizeX + 3 * fieldIncX) / Font_Big.width, Font_Big,
			Label::Orientation::CENTER);
	c->attach(l, COORDS(fieldOffsetX, 7));
	for (uint8_t i = 0; i < 3; i++) {
		for (uint8_t j = 0; j < 3; j++) {
			uint8_t number = 1 + i + j * 3;
			char name[2] = { (char) (number + '0'), 0 };
			auto button = new Button(name, Font_Big,
					pmf_cast<void (*)(void*, Widget *w), ValueInput,
							&ValueInput::NumberPressed>::cfn, this,
					COORDS(fieldSizeX, fieldSizeY));
			c->attach(button,
					COORDS(fieldOffsetX + i * fieldIncX,
							fieldOffsetY + j * fieldIncY));
		}
	}
	auto zero = new Button("0", Font_Big,
			pmf_cast<void (*)(void*, Widget *w), ValueInput,
					&ValueInput::NumberPressed>::cfn, this,
			COORDS(fieldSizeX, fieldSizeY));

	if (unit == Unit::Hex) {
		strcpy(string, "0x");
		inputIndex = 2;
		l->setText(string);
		for (uint8_t i = 3; i < 6; i++) {
			for (uint8_t j = 0; j < 2; j++) {
				char name[2] = { (char) ('A' + i - 3 + j * 3), 0 };
				auto button = new Button(name, Font_Big,
						pmf_cast<void (*)(void*, Widget *w), ValueInput,
								&ValueInput::NumberPressed>::cfn, this,
						COORDS(fieldSizeX, fieldSizeY));
				c->attach(button,
						COORDS(fieldOffsetX + i * fieldIncX,
								fieldOffsetY + j * fieldIncY));
			}
		}
		c->attach(zero,
				COORDS(fieldOffsetX + 3 * fieldIncX,
						fieldOffsetY + 2 * fieldIncY));
		auto enter = new Button("Enter", Font_Big,
				pmf_cast<void (*)(void*, Widget *w), ValueInput,
						&ValueInput::UnitPressed>::cfn, this,
				COORDS(fieldSizeX + fieldIncX, fieldSizeY));
		c->attach(enter,
				COORDS(fieldOffsetX + 4 * fieldIncX, fieldOffsetY + 2 * fieldIncY));
	} else {
		auto sign = new Button("+/-", Font_Big,
				pmf_cast<void (*)(void*, Widget *w), ValueInput,
						&ValueInput::ChangeSign>::cfn, this,
				COORDS(fieldSizeX, fieldSizeY));
		c->attach(sign,
				COORDS(fieldOffsetX + 3 * fieldIncX,
						fieldOffsetY + 0 * fieldIncY));
		c->attach(zero,
				COORDS(fieldOffsetX + 3 * fieldIncX,
						fieldOffsetY + 1 * fieldIncY));
		dot = new Button(".", Font_Big,
				pmf_cast<void (*)(void*, Widget *w), ValueInput,
						&ValueInput::NumberPressed>::cfn, this,
				COORDS(fieldSizeX, fieldSizeY));
		c->attach(dot,
				COORDS(fieldOffsetX + 3 * fieldIncX,
						fieldOffsetY + 2 * fieldIncY));
		uint16_t yOffset = fieldOffsetY;
		while (*unit) {
			const char *name = (*unit)->name;
			if (unit == Unit::None) {
				name = "Enter";
			}
			auto button = new Button(name, Font_Big,
					pmf_cast<void (*)(void*, Widget *w), ValueInput,
							&ValueInput::UnitPressed>::cfn, this,
					COORDS(fieldSizeX + fieldIncX, fieldSizeY));
			c->attach(button, COORDS(fieldOffsetX + 4 * fieldIncX, yOffset));
			yOffset += fieldIncY;
			unit++;
		}
	}
	auto backspace = new Button("BACKSPACE", Font_Big,
			pmf_cast<void (*)(void*, Widget *w), ValueInput,
					&ValueInput::Backspace>::cfn, this,
			COORDS(fieldSizeX + 3 * fieldIncX, 35));
	c->attach(backspace,
			COORDS(fieldOffsetX, fieldOffsetY + 3 * fieldIncY));
	auto abort = new Button("Abort", Font_Big, [](void *ptr, Widget *w) {
		ValueInput *v = (ValueInput*) ptr;
		auto cb_buf = v->cb;
		auto ptr_buf = v->ptr;
		delete v;
		if (cb_buf) {
			cb_buf(ptr_buf, false);
		}
	}, this, COORDS(fieldSizeX + fieldIncX, 35));
	c->attach(abort,
			COORDS(fieldOffsetX + 4 * fieldIncX, fieldOffsetY + 3 * fieldIncY));
	w->setMainWidget(c);
	LOG(Log_GUI, LevelInfo, "Created value dialog, free heap: %lu",
			xPortGetFreeHeapSize());
}

ValueInput::~ValueInput() {
	delete w;
	LOG(Log_GUI, LevelInfo, "Removed value dialog, free heap: %lu",
			xPortGetFreeHeapSize());
}

void ValueInput::UnitPressed(Widget* w) {
	if (unit == Unit::Hex || unit == Unit::None) {
		*value = strtol(string, nullptr, 0);
	} else {
		Button *b = (Button*) w;
		// find correct unit
		while (*unit) {
			if (!strncmp(b->getName(), (*unit)->name, strlen(b->getName()))) {
				// found the correct unit
				*value = Unit::ValueFromString(string, (*unit)->factor);
				break;
			}
			unit++;
		}
	}
	auto cb_buf = cb;
	auto ptr_buf = ptr;
	delete this;
	if (cb_buf) {
		cb_buf(ptr_buf, true);
	}
}

void ValueInput::NumberPressed(Widget* w) {
	Button *b = (Button*) w;
	if (inputIndex < maxInputLength) {
		string[inputIndex++] = b->getName()[0];
		if (b->getName()[0] == '.') {
			b->setSelectable(false);
		}
		l->setText(string);
	}
}

void ValueInput::ChangeSign(Widget* w) {
	if(string[0] == '-') {
		memmove(string, &string[1], --inputIndex);
		string[inputIndex] = 0;
		l->setText(string);
	} else if(inputIndex < maxInputLength) {
		memmove(&string[1], string, inputIndex++);
		string[0] = '-';
		l->setText(string);
	}
}

void ValueInput::Backspace(Widget* w) {
	if (inputIndex > 2 || (inputIndex > 0 && unit != Unit::Hex)) {
		inputIndex--;
		if (string[inputIndex] == '.') {
			dot->setSelectable(true);
		}
		string[inputIndex] = 0;
		l->setText(string);
	}
}
