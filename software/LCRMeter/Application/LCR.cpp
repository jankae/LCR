#include "LCR.hpp"
#include <cstdint>
#include "gui.hpp"
#include "HardwareLimits.hpp"
#include "Frontend.hpp"
#include <math.h>

static int32_t measurementFrequency = 1000;
static int32_t biasVoltage = 0;
static bool measurementUpdated = false;
static uint32_t measurementAverages = 10;
static bool newMeasurement = false;
static Frontend::Result measurementResult;
static TaskHandle_t handle = nullptr;

// GUI elements
Custom *cResult;

enum class DisplayMode : uint8_t {
			AUTO = 0x00,
			SERIES = 0x01,
			PARALLEL = 0x02,
};

static DisplayMode displayMode = DisplayMode::AUTO;

enum class ImpedanceType : uint8_t {
	CAPACITANCE = 0x00,
	INDUCTANCE = 0x01,
};

using Result = struct {
	Frontend::Result frontend;
	float real, imag;
	DisplayMode mode;
	ImpedanceType type;
	union {
		struct {
			float capacitance;
		} C;
		struct {
			float inductance;
		} L;
	};
};

static Result lastMeasurement;

enum class Component : uint8_t {
	RESISTOR,
	CAPACITOR,
	INDUCTOR,
};

static void drawComponent(Component c, coords_t startpos, uint16_t len) {
	uint16_t componentSize = 0;
	display_SetForeground(COLOR_BLACK);
	display_SetBackground(COLOR_BG_DEFAULT);
	// Draw component
	switch(c) {
	case Component::RESISTOR: {
		componentSize = 40;
		uint16_t startx = startpos.x + len / 2 - componentSize / 2;
		display_Rectangle(startx, startpos.y - componentSize / 4,
				startx + componentSize, startpos.y + componentSize / 4);
		display_Rectangle(startx + 1, startpos.y - componentSize / 4 + 1,
				startx + componentSize - 1, startpos.y + componentSize / 4 - 1);
		display_Rectangle(startx + 2, startpos.y - componentSize / 4 + 2,
				startx + componentSize - 2, startpos.y + componentSize / 4 - 2);
	}
		break;
	case Component::INDUCTOR: {
		componentSize = 40;
		uint16_t startx = startpos.x + len / 2 - componentSize / 2;
		display_RectangleFull(startx, startpos.y - componentSize / 4,
				startx + componentSize, startpos.y + componentSize / 4);
	}
		break;
	case Component::CAPACITOR: {
		constexpr uint16_t height = 20;
		componentSize = 12;
		uint16_t startx = startpos.x + len / 2 - componentSize / 2;
		display_RectangleFull(startx, startpos.y - height / 2, startx + 3,
				startpos.y + height / 2);
		display_RectangleFull(startx + componentSize - 3, startpos.y - height / 2,
				startx + componentSize, startpos.y + height / 2);
	}
		break;
	}
	// draw leads
	display_RectangleFull(startpos.x, startpos.y - 1,
			startpos.x + len / 2 - componentSize / 2, startpos.y + 1);
	display_RectangleFull(startpos.x + len / 2 + componentSize / 2,
			startpos.y - 1, startpos.x + len, startpos.y + 1);
}

static void drawResult(Widget&, coords_t pos) {
	static DisplayMode mode = DisplayMode::AUTO;
	static ImpedanceType type = ImpedanceType::CAPACITANCE;

	coords_t SchematicTopLeft = COORDS(0, 50) + pos;
	coords_t SchematicBottomRight = COORDS(200, 200) + pos;
	coords_t SchematicCenter = COORDS(
			(SchematicTopLeft.x + SchematicBottomRight.x) / 2,
			(SchematicTopLeft.y + SchematicBottomRight.y) / 2);
	if (mode != lastMeasurement.mode || type != lastMeasurement.type) {
		// Schematic changed, clear drawing area
		display_SetForeground(COLOR_BG_DEFAULT);
		display_RectangleFull(SchematicTopLeft.x, SchematicTopLeft.y,
				SchematicBottomRight.x, SchematicBottomRight.y);
		mode = lastMeasurement.mode;
		type = lastMeasurement.type;
	}
	// draw schematic
	Component c;
	if (type == ImpedanceType::CAPACITANCE) {
		c = Component::CAPACITOR;
	} else {
		c = Component::INDUCTOR;
	}
	if (mode == DisplayMode::SERIES) {
		// draw components
		drawComponent(Component::RESISTOR,
				COORDS(SchematicTopLeft.x, SchematicCenter.y),
				SchematicCenter.x - SchematicTopLeft.x);
		drawComponent(c, COORDS(SchematicCenter.x, SchematicCenter.y),
				SchematicBottomRight.x - SchematicCenter.x);
	} else {
		constexpr uint16_t DistanceFromCenterLine = 20;
		// draw components
		drawComponent(Component::RESISTOR,
				COORDS((SchematicTopLeft.x + SchematicCenter.x) / 2,
						SchematicCenter.y - DistanceFromCenterLine),
				SchematicCenter.x - SchematicTopLeft.x);
		drawComponent(c,
				COORDS((SchematicTopLeft.x + SchematicCenter.x) / 2,
						SchematicCenter.y + DistanceFromCenterLine),
				SchematicBottomRight.x - SchematicCenter.x);
		// draw connecting lines
		// end lines
		display_RectangleFull(SchematicTopLeft.x, SchematicCenter.y - 1,
				(SchematicTopLeft.x + SchematicCenter.x) / 2,
				SchematicCenter.y + 1);
		display_RectangleFull((SchematicBottomRight.x + SchematicCenter.x) / 2,
				SchematicCenter.y - 1, SchematicBottomRight.x,
				SchematicCenter.y + 1);
		// parallel bars
		display_RectangleFull((SchematicTopLeft.x + SchematicCenter.x) / 2 - 1,
				SchematicCenter.y - DistanceFromCenterLine,
				(SchematicTopLeft.x + SchematicCenter.x) / 2 + 1,
				SchematicCenter.y + DistanceFromCenterLine);
		display_RectangleFull(
				(SchematicBottomRight.x + SchematicCenter.x) / 2 - 1,
				SchematicCenter.y - DistanceFromCenterLine,
				(SchematicBottomRight.x + SchematicCenter.x) / 2 + 1,
				SchematicCenter.y + DistanceFromCenterLine);
	}
}

static Result CalculateComponentValues(Frontend::Result f) {
	Result res;
	res.frontend = f;
	if (f.Phase >= 0.0f) {
		res.type = ImpedanceType::INDUCTANCE;
	} else {
		res.type = ImpedanceType::CAPACITANCE;
	}
	if (displayMode == DisplayMode::AUTO) {
		// Change mode depending on result
		if (res.type == ImpedanceType::INDUCTANCE) {
			if (f.Phase <= 45.0f) {
				// resistance dominates
				res.mode = DisplayMode::SERIES;
			} else {
				// inductance dominates
				res.mode = DisplayMode::PARALLEL;
			}
		} else {
			if (f.Phase >= -45.0f) {
				// resistance dominates
				res.mode = DisplayMode::PARALLEL;
			} else {
				// inductance dominates
				res.mode = DisplayMode::SERIES;
			}
		}
	} else {
		res.mode = displayMode;
	}
	if(res.mode == DisplayMode::SERIES) {
		// Assuming series connection of components
		res.real = f.I;
		res.imag = f.Q;
	} else {
		// Assuming parallel connection of components
		res.real = (f.I * f.I + f.Q + f.Q) / f.I;
		res.imag = (f.I * f.I + f.Q + f.Q) / f.Q;
	}
	if(res.type == ImpedanceType::CAPACITANCE) {
		// Calculate capacitor values
		res.C.capacitance = -1.0f / (2 * M_PI * f.frequency * res.imag);
	} else {
		// Calculate inductor value
		res.L.inductance = res.imag / (2 * M_PI * f.frequency);
	}
	return res;
}

static void measurementCallback(void*, Frontend::Result result) {
	measurementResult = result;
	if (handle) {
		xTaskNotify(handle, 0, eNoAction);
	}
}

bool LCR::Init() {
	auto callback_setTrueNotify = [](void *ptr, Widget*) {
		bool *flag = (bool*) ptr;
		*flag = true;
		if (handle) {
			xTaskNotify(handle, 0, eNoAction);
		}
	};

	Container *c = new Container(SIZE(DISPLAY_WIDTH, DISPLAY_HEIGHT));
	Menu *mainmenu = new Menu("", SIZE(70, DISPLAY_HEIGHT));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Frequency", &measurementFrequency, Unit::Frequency,
					callback_setTrueNotify, &measurementUpdated, HardwareLimits::MinFrequency,
					HardwareLimits::MaxFrequency));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Bias", &biasVoltage, Unit::Voltage,
					callback_setTrueNotify, &measurementUpdated, HardwareLimits::MinBiasVoltage,
					HardwareLimits::MaxBiasVoltage));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Averages", &measurementAverages, Unit::None,
					callback_setTrueNotify, &measurementUpdated, 1, 100));
	static constexpr char *mode_items[] = {
			"AUTO", "SERIES", "PARALLEL", nullptr
	};
	mainmenu->AddEntry(
			new MenuChooser("Model", mode_items, (uint8_t*) &displayMode,
					nullptr, nullptr, false));
	c->attach(mainmenu, COORDS(DISPLAY_WIDTH - mainmenu->getSize().x, 0));
	cResult = new Custom(
			SIZE(DISPLAY_WIDTH - mainmenu->getSize().x, DISPLAY_HEIGHT),
			drawResult, nullptr);
	c->attach(cResult, COORDS(0, 0));
	c->requestRedrawFull();
	GUI::Init(*c);

	Frontend::SetCallback(measurementCallback);
	return true;
}

static void ConfigureFrontendMeasurement() {
	Frontend::Settings s;
	s.biasVoltage = biasVoltage;
	s.frequency = measurementFrequency;
	s.averages = measurementAverages;
	s.range = Frontend::Range::AUTO;
	Frontend::Start(s);
}

void LCR::Run() {
	handle = xTaskGetCurrentTaskHandle();
	//ConfigureFrontendMeasurement();
	while (1) {
		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		if (measurementUpdated) {
			ConfigureFrontendMeasurement();
			measurementUpdated = false;
		}
		if (newMeasurement) {
			lastMeasurement = CalculateComponentValues(measurementResult);
			cResult->requestRedraw();
			newMeasurement = false;
		}
	}
}
