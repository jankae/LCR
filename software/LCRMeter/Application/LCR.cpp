#include "LCR.hpp"
#include <cstdint>
#include "gui.hpp"
#include "HardwareLimits.hpp"

static int32_t measurementFrequency = 1000;
static int32_t biasVoltage = 0;
static bool measurementUpdated = false;

bool LCR::Init() {
	auto callback_setTrue = [](void *ptr, Widget*) {
		bool *flag = (bool*) ptr;
		*flag = true;
	};

	Container *c = new Container(SIZE(DISPLAY_WIDTH, DISPLAY_HEIGHT));
	Menu *mainmenu = new Menu("", SIZE(70, DISPLAY_HEIGHT));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Frequency", &measurementFrequency, Unit::Frequency,
					callback_setTrue, &measurementUpdated, HardwareLimits::MinFrequency,
					HardwareLimits::MaxFrequency));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Bias", &biasVoltage, Unit::Voltage,
					callback_setTrue, &measurementUpdated, HardwareLimits::MinBiasVoltage,
					HardwareLimits::MaxBiasVoltage));
	c->attach(mainmenu, COORDS(DISPLAY_WIDTH - mainmenu->getSize().x, 0));
	GUI::Init(*c);

	return true;
}

void LCR::Run() {
	while(1) {
		vTaskDelay(1000);
	}
}
