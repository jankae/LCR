#pragma once

#include <cstdint>
#include "window.hpp"
#include "progressbar.hpp"

class ProgressDialog {
public:
	ProgressDialog(const char *title, uint16_t minWidth);
	~ProgressDialog();
	void SetPercentage(uint8_t percentage);
private:
	Window *w;
	ProgressBar *p;
};
