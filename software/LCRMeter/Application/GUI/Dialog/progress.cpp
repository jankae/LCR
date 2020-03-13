#include "progress.hpp"

ProgressDialog::ProgressDialog(const char* title, uint16_t minWidth) {
	uint16_t width = strlen(title) * Font_Big.width + 5;
	if(minWidth > width) {
		width = minWidth;
	}
	w = new Window(title, Font_Big, COORDS(width, Font_Big.height + 30));
	p = new ProgressBar(w->getAvailableArea());
	w->setMainWidget(p);
}

ProgressDialog::~ProgressDialog() {
	delete w;
}

void ProgressDialog::SetPercentage(uint8_t percentage) {
	p->setState(percentage);
}
