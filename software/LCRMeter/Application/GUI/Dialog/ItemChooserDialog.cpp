#include "ItemChooserDialog.hpp"

#include "cast.hpp"
//#include "buttons.h"
#include "gui.hpp"
#include "Unit.hpp"
#include "util.h"

ItemChooserDialog::ItemChooserDialog(const char* title,
		const char* const * items, uint8_t initial_selection, Callback cb,
		void* ptr) {
	this->cb = cb;
	this->ptr = ptr;
	value = initial_selection;
	w = new Window(title, Font_Big, COORDS(240, 170));
	auto c = new Container(w->getAvailableArea());
	auto i = new ItemChooser(items, &value, Font_Big, 6, c->getSize().x);
	auto ok = new Button("OK", Font_Big,
			pmf_cast<void (*)(void*, Widget *w), ItemChooserDialog,
					&ItemChooserDialog::ButtonPressed>::cfn, this,
			COORDS(c->getSize().x / 2 - 10, 40));
	auto abort = new Button("ABORT", Font_Big,
			pmf_cast<void (*)(void*, Widget *w), ItemChooserDialog,
					&ItemChooserDialog::ButtonPressed>::cfn, this,
			COORDS(c->getSize().x / 2 - 10, 40));
	auto ec = new EventCatcher(i, [](GUIEvent_t * const ev) -> bool {
//		if(ev->type == EVENT_BUTTON_CLICKED) {
//			if(ev->button & (BUTTON_ENCODER | BUTTON_ESC)) {
//				return true;
//			}
//		}
		return false;
	}, pmf_cast<void (*)(void*, Widget *w, GUIEvent_t *ev), ItemChooserDialog,
	&ItemChooserDialog::EventCaught>::cfn, this);
	c->attach(ec, COORDS(0, 0));
	c->attach(abort, COORDS(2, c->getSize().y - abort->getSize().y - 2));
	c->attach(ok,
			COORDS(c->getSize().x - abort->getSize().x - 2,
					c->getSize().y - abort->getSize().y - 2));
	w->setMainWidget(c);
	i->select();
}

ItemChooserDialog::~ItemChooserDialog() {
	delete w;
}

void ItemChooserDialog::ButtonPressed(Widget* w) {
	Button *b = (Button*) w;
	bool ret = false;
	if (!strcmp("OK", b->getName())) {
		ret = true;
	}
	auto cb_buf = cb;
	auto ptr_buf = ptr;
	auto val_buf = value;
	delete this;
	if (cb_buf) {
		cb_buf(ptr_buf, ret, val_buf);
	}
}

void ItemChooserDialog::EventCaught(Widget *w, GUIEvent_t *ev) {
//	if (ev->type == EVENT_BUTTON_CLICKED) {
//		bool ret = false;
//		if (ev->button & (BUTTON_ENCODER)) {
//			ret = true;
//		}
//		auto cb_buf = cb;
//		auto ptr_buf = ptr;
//		auto val_buf = value;
//		delete this;
//		if (cb_buf) {
//			cb_buf(ptr_buf, ret, val_buf);
//		}
//	}
}
