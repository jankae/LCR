#include "LCR.hpp"
#include <cstdint>
#include "gui.hpp"
#include "HardwareLimits.hpp"
#include "Frontend.hpp"
#include <math.h>
#include "log.h"
#include "touch.h"
#include <complex>
#include "Sweep.hpp"

using namespace std;

#define Log_LCR (LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

static int32_t measurementFrequency = 1000;
static int32_t biasVoltage = 0;
static int32_t excitationVoltage = 100000;
static bool measurementUpdated = false;
static uint32_t measurementAverages = 10;
static bool newMeasurement = false;
static Frontend::Result measurementResult;
static TaskHandle_t handle = nullptr;

static complex<float> Zopen, Zshort;
static bool leadCompensation = false;

// GUI elements
Custom *cResult;
static Menu *mainmenu;
static Sweep *sweep;

static LCR::DisplayMode displayMode = LCR::DisplayMode::AUTO;
static LCR::Result lastMeasurement;

enum class Component : uint8_t {
	RESISTOR,
	CAPACITOR,
	INDUCTOR,
};

static void drawComponent(Component c, coords_t startpos, uint16_t len) {
	uint16_t componentSize = 0;
	display_SetForeground(LCR::SchematicColor);
	display_SetBackground(COLOR_BG_DEFAULT);
	// Draw component
	switch (c) {
	case Component::RESISTOR: {
		componentSize = 40;
		uint16_t startx = startpos.x + len / 2 - componentSize / 2;
		display_Rectangle(startx, startpos.y - componentSize / 4, startx + componentSize,
				startpos.y + componentSize / 4);
		display_Rectangle(startx + 1, startpos.y - componentSize / 4 + 1, startx + componentSize - 1,
				startpos.y + componentSize / 4 - 1);
		display_Rectangle(startx + 2, startpos.y - componentSize / 4 + 2, startx + componentSize - 2,
				startpos.y + componentSize / 4 - 2);
	}
		break;
	case Component::INDUCTOR: {
		componentSize = 40;
		uint16_t startx = startpos.x + len / 2 - componentSize / 2;
		display_RectangleFull(startx, startpos.y - componentSize / 4, startx + componentSize,
				startpos.y + componentSize / 4);
	}
		break;
	case Component::CAPACITOR: {
		constexpr uint16_t height = 20;
		componentSize = 12;
		uint16_t startx = startpos.x + len / 2 - componentSize / 2;
		display_RectangleFull(startx, startpos.y - height / 2, startx + 3, startpos.y + height / 2);
		display_RectangleFull(startx + componentSize - 3, startpos.y - height / 2, startx + componentSize,
				startpos.y + height / 2);
	}
		break;
	}
	// draw leads
	display_RectangleFull(startpos.x, startpos.y - 1, startpos.x + len / 2 - componentSize / 2, startpos.y + 1);
	display_RectangleFull(startpos.x + len / 2 + componentSize / 2, startpos.y - 1, startpos.x + len, startpos.y + 1);
}

static void drawResult(Widget &w, coords_t pos) {
	static LCR::DisplayMode mode = LCR::DisplayMode::AUTO;
	static LCR::ImpedanceType ImpType = LCR::ImpedanceType::CAPACITANCE;
	static Frontend::ResultType ResType = Frontend::ResultType::OpenLeads;

	coords_t SchematicTopLeft = COORDS(0, 0) + pos;
	coords_t SchematicBottomRight = COORDS(w.getSize().x - 1, 100) + pos;
	coords_t SchematicCenter = COORDS((SchematicTopLeft.x + SchematicBottomRight.x) / 2,
			(SchematicTopLeft.y + SchematicBottomRight.y) / 2);
	if (mode != lastMeasurement.mode || ImpType != lastMeasurement.type || ResType != lastMeasurement.frontend.type) {
		// Schematic changed, clear drawing area
		display_SetForeground(COLOR_BG_DEFAULT);
		display_RectangleFull(SchematicTopLeft.x, SchematicTopLeft.y, SchematicBottomRight.x, SchematicBottomRight.y);
		mode = lastMeasurement.mode;
		ImpType = lastMeasurement.type;
		ResType = lastMeasurement.frontend.type;
	}
	display_SetForeground(COLOR_BLACK);
	display_SetBackground(COLOR_BG_DEFAULT);
	display_SetFont(Font_Big);
	switch (lastMeasurement.frontend.type) {
	case Frontend::ResultType::Valid: {
		// Show component values
		char val[22];
		Unit::SIStringFromFloat(val, 7, real(lastMeasurement.Z));
		strcat(val, "Ohm");
		display_SetForeground(LCR::MeasurmentValueColor);
		display_AutoCenterString(val, pos + COORDS(0, 3), COORDS(pos.x + w.getSize().x, pos.y + 19));
		if (ImpType == LCR::ImpedanceType::CAPACITANCE) {
			Unit::SIStringFromFloat(val, 7, lastMeasurement.C.capacitance);
			strcat(val, "F Q:");
		} else {
			Unit::SIStringFromFloat(val, 7, lastMeasurement.L.inductance);
			strcat(val, "H Q:");
		}
		// add quality factor to string
		uint8_t start = strlen(val);
		Unit::SIStringFromFloat(&val[start], 7, lastMeasurement.qualityFactor, ' ');
		display_AutoCenterString(val, COORDS(pos.x, pos.y + 84), COORDS(pos.x + w.getSize().x, pos.y + 100));
		// draw schematic
		constexpr uint16_t padLeftRight = 10;
		Component c;
		if (ImpType == LCR::ImpedanceType::CAPACITANCE) {
			c = Component::CAPACITOR;
		} else {
			c = Component::INDUCTOR;
		}
		if (mode == LCR::DisplayMode::SERIES) {
			// draw components
			drawComponent(Component::RESISTOR, COORDS(SchematicTopLeft.x + padLeftRight, SchematicCenter.y),
					SchematicCenter.x - SchematicTopLeft.x);
			drawComponent(c, COORDS(SchematicCenter.x, SchematicCenter.y),
					SchematicBottomRight.x - SchematicCenter.x - padLeftRight);
		} else {
			constexpr uint16_t DistanceFromCenterLine = 20;
			// draw components
			drawComponent(Component::RESISTOR,
					COORDS((SchematicTopLeft.x + SchematicCenter.x) / 2, SchematicCenter.y - DistanceFromCenterLine),
					SchematicCenter.x - SchematicTopLeft.x);
			drawComponent(c,
					COORDS((SchematicTopLeft.x + SchematicCenter.x) / 2, SchematicCenter.y + DistanceFromCenterLine),
					SchematicBottomRight.x - SchematicCenter.x);
			// draw connecting lines
			// end lines
			display_RectangleFull(SchematicTopLeft.x + padLeftRight, SchematicCenter.y - 1,
					(SchematicTopLeft.x + SchematicCenter.x) / 2, SchematicCenter.y + 1);
			display_RectangleFull((SchematicBottomRight.x + SchematicCenter.x) / 2, SchematicCenter.y - 1,
					SchematicBottomRight.x - padLeftRight, SchematicCenter.y + 1);
			// parallel bars
			display_RectangleFull((SchematicTopLeft.x + SchematicCenter.x) / 2 - 1,
					SchematicCenter.y - DistanceFromCenterLine, (SchematicTopLeft.x + SchematicCenter.x) / 2 + 1,
					SchematicCenter.y + DistanceFromCenterLine);
			display_RectangleFull((SchematicBottomRight.x + SchematicCenter.x) / 2 - 1,
					SchematicCenter.y - DistanceFromCenterLine, (SchematicBottomRight.x + SchematicCenter.x) / 2 + 1,
					SchematicCenter.y + DistanceFromCenterLine);
		}
	}
		break;
	case Frontend::ResultType::OpenLeads:
		display_AutoCenterString("NO LEADS", SchematicTopLeft, SchematicBottomRight);
		break;
	case Frontend::ResultType::Overrange: {
		char val[22];
		strcpy(val, "|Z| > ");
		Unit::SIStringFromFloat(&val[strlen(val)], 7, lastMeasurement.frontend.LimitHigh);
		strcat(val, "Ohm");
		display_AutoCenterString(val, SchematicTopLeft, SchematicBottomRight);
	}
		break;
	case Frontend::ResultType::Underrange: {
		char val[22];
		strcpy(val, "|Z| < ");
		Unit::SIStringFromFloat(&val[strlen(val)], 7, lastMeasurement.frontend.LimitLow);
		strcat(val, "Ohm");
		display_AutoCenterString(val, SchematicTopLeft, SchematicBottomRight);
	}
		break;
	}

	// Draw ADC ranges at bottom
	constexpr uint16_t xSpaceText = 75;
	constexpr uint16_t xPadLeft = 40;
	coords_t ADCRangeTopLeft = COORDS(0, w.getSize().y - 20) + pos;
	coords_t ADCRangeBottomRight = pos + w.getSize();
	// Voltage
	display_SetForeground(COLOR_BLACK);
	display_SetFont(Font_Medium);
	display_String(2, ADCRangeTopLeft.y + 1, "ADC U:");
	display_Rectangle(ADCRangeTopLeft.x + xPadLeft, ADCRangeTopLeft.y, ADCRangeBottomRight.x - xSpaceText,
			ADCRangeTopLeft.y + 8);
	uint8_t used_range = constrain_int16_t(lastMeasurement.frontend.usedRangeU, 0, 100);
	uint16_t x_range = util_Map(used_range, 0, 100, xPadLeft + 1, ADCRangeBottomRight.x - xSpaceText - 1);
	display_SetForeground(lastMeasurement.frontend.usedRangeU <= 100 ? LCR::BarColor : COLOR_RED);
	display_RectangleFull(xPadLeft + 1, ADCRangeTopLeft.y + 1, x_range, ADCRangeTopLeft.y + 7);
	display_SetForeground(COLOR_BG_DEFAULT);
	display_RectangleFull(x_range + 1, ADCRangeTopLeft.y + 1, ADCRangeBottomRight.x - xSpaceText - 1,
			ADCRangeTopLeft.y + 7);
	char val[15];
	if (lastMeasurement.frontend.clippedU) {
		display_SetForeground(COLOR_RED);
		strcpy(val, "--CLIPPED--");
	} else {
		Unit::SIStringFromFloat(val, 7, lastMeasurement.frontend.RMS_U);
		strcat(val, "Vrms");
		display_SetForeground(COLOR_BLACK);
	}
	display_String(ADCRangeBottomRight.x - xSpaceText + 2, ADCRangeTopLeft.y + 1, val);

	// Current
	display_SetForeground(COLOR_BLACK);
	display_String(2, ADCRangeTopLeft.y + 11, "ADC I:");
	display_Rectangle(ADCRangeTopLeft.x + xPadLeft, ADCRangeTopLeft.y + 10, ADCRangeBottomRight.x - xSpaceText,
			ADCRangeTopLeft.y + 18);
	used_range = constrain_int16_t(lastMeasurement.frontend.usedRangeI, 0, 100);
	x_range = util_Map(used_range, 0, 100, xPadLeft + 1, ADCRangeBottomRight.x - xSpaceText - 1);
	display_SetForeground(lastMeasurement.frontend.usedRangeI <= 100 ? LCR::BarColor : COLOR_RED);
	display_RectangleFull(xPadLeft + 1, ADCRangeTopLeft.y + 11, x_range, ADCRangeTopLeft.y + 17);
	display_SetForeground(COLOR_BG_DEFAULT);
	display_RectangleFull(x_range + 1, ADCRangeTopLeft.y + 11, ADCRangeBottomRight.x - xSpaceText - 1,
			ADCRangeTopLeft.y + 17);
	if (lastMeasurement.frontend.clippedI) {
		display_SetForeground(COLOR_RED);
		strcpy(val, "--CLIPPED--");
	} else {
		Unit::SIStringFromFloat(val, 7, lastMeasurement.frontend.RMS_I);
		strcat(val, "Arms");
		display_SetForeground(COLOR_BLACK);
	}
	display_String(ADCRangeBottomRight.x - xSpaceText + 2, ADCRangeTopLeft.y + 11, val);
}

static LCR::Result CalculateComponentValues(Frontend::Result f) {
	LCR::Result res;
	if (leadCompensation) {
		f.Z = (f.Z - Zshort) / (complex<float>(1, 0) - f.Z / Zopen);
	}
	res.frontend = f;
	float phase = 180 * arg(res.frontend.Z) / M_PI;
	if (phase >= 0.0f) {
		res.type = LCR::ImpedanceType::INDUCTANCE;
	} else {
		res.type = LCR::ImpedanceType::CAPACITANCE;
	}
	if (displayMode == LCR::DisplayMode::AUTO) {
		if (phase < 5.0f && phase > -5.0f) {
			// resistance dominates, always choose parallel model
			res.mode = LCR::DisplayMode::PARALLEL;
		} else {
			if (abs(res.frontend.Z) < 1000.0f) {
				res.mode = LCR::DisplayMode::SERIES;
			} else {
				res.mode = LCR::DisplayMode::PARALLEL;
			}
		}
	} else {
		res.mode = displayMode;
	}
	if (res.mode == LCR::DisplayMode::SERIES) {
		// Assuming series connection of components
		res.Z = res.frontend.Z;
	} else {
		// Assuming parallel connection of components
		res.Z = complex<float>(norm(res.frontend.Z) / real(res.frontend.Z), norm(res.frontend.Z) / imag(res.frontend.Z));
	}
	if (res.type == LCR::ImpedanceType::CAPACITANCE) {
		// Calculate capacitor values
		res.C.capacitance = -1.0f / (2 * M_PI * f.frequency * imag(res.Z));
		res.qualityFactor = -imag(f.Z) / real(f.Z);
	} else {
		// Calculate inductor value
		res.L.inductance = imag(res.Z) / (2 * M_PI * f.frequency);
		res.qualityFactor = imag(f.Z) / real(f.Z);
	}
	return res;
}

static void measurementCallback(void*, Frontend::Result result) {
	measurementResult = result;
	newMeasurement = true;
	if (handle) {
		xTaskNotify(handle, 0, eNoAction);
	}
}

bool LCR::Init() {
	auto callback_setTrueNotify = [](void *ptr, Widget*) {
		if (ptr) {
			bool *flag = (bool*) ptr;
			*flag = true;
		}
		if (handle) {
			xTaskNotify(handle, 0, eNoAction);
		}
	};

	Container *c = new Container(SIZE(DISPLAY_WIDTH, DISPLAY_HEIGHT));
	mainmenu = new Menu("", SIZE(70, DISPLAY_HEIGHT));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Frequency", &measurementFrequency, Unit::Frequency, callback_setTrueNotify,
					&measurementUpdated, HardwareLimits::MinFrequency, HardwareLimits::MaxFrequency));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Bias", &biasVoltage, Unit::Voltage, callback_setTrueNotify, &measurementUpdated,
					HardwareLimits::MinBiasVoltage, HardwareLimits::MaxBiasVoltage));
	mainmenu->AddEntry(
			new MenuValue<int32_t>("Averages", &measurementAverages, Unit::None, callback_setTrueNotify,
					&measurementUpdated, 1, 100));

	sweep = new Sweep(SIZE(DISPLAY_WIDTH - mainmenu->getSize().x, DISPLAY_HEIGHT - 10), *mainmenu);
	sweep->setVisible(false);
	c->attach(sweep, COORDS(0, 0));

	auto advancedMenu = new Menu("Advanced\nSettings", mainmenu->getSize());

	static constexpr char *mode_items[] = { "AUTO", "SERIES", "PARALLEL", nullptr };

	advancedMenu->AddEntry(new MenuChooser("Model", mode_items, (uint8_t*) &displayMode, nullptr, nullptr, false));
	advancedMenu->AddEntry(
			new MenuValue<int32_t>("Excitation", &excitationVoltage, Unit::Voltage, callback_setTrueNotify,
					&measurementUpdated, HardwareLimits::MinExcitationVoltage, HardwareLimits::MaxExcitationVoltage));
	advancedMenu->AddEntry(new MenuBool("O/S Comp.", &leadCompensation, callback_setTrueNotify, nullptr));
	advancedMenu->AddEntry(new MenuBack());

	mainmenu->AddEntry(advancedMenu);

	Menu *systemmenu = new Menu("System", mainmenu->getSize());
	mainmenu->AddEntry(systemmenu);
	systemmenu->AddEntry(
			new MenuAction("Calibrate\nFrontend",
					[](void*, Widget*) {
						Dialog::MessageBox("Start Calibration?", Font_Big, "Short all inputs", Dialog::MsgBox::ABORT_OK,
								[](Dialog::Result r) {
									if (r == Dialog::Result::OK) {
										Frontend::Calibrate();
									}
								}, 0);
					}, nullptr));
	systemmenu->AddEntry(new MenuAction("Calibrate\nTouch", [](void*, Widget*) {
		touch_Calibrate();
	}, nullptr));
	systemmenu->AddEntry(new MenuBack());
	c->attach(mainmenu, COORDS(DISPLAY_WIDTH - mainmenu->getSize().x, 0));
	cResult = new Custom(SIZE(DISPLAY_WIDTH - mainmenu->getSize().x, DISPLAY_HEIGHT - 10), drawResult, nullptr);
	c->attach(cResult, COORDS(0, 0));
	auto p = new ProgressBar(COORDS(DISPLAY_WIDTH - mainmenu->getSize().x - 10, 9), LCR::BarColor);
	c->attach(p, COORDS(5, DISPLAY_HEIGHT - 10));
	c->requestRedrawFull();
	GUI::Init(*c);

	Frontend::SetCallback(measurementCallback);
	Frontend::SetAcquisitionProgressBar(p);
	return true;
}

static void ConfigureFrontendMeasurement() {
	Frontend::Settings s;
	s.biasVoltage = biasVoltage;
	s.frequency = measurementFrequency;
	s.averages = measurementAverages;
	s.excitationVoltage = excitationVoltage;
	s.range = Frontend::Range::AUTO;
	Frontend::Start(s);
}

void LCR::Run() {
	handle = xTaskGetCurrentTaskHandle();
	ConfigureFrontendMeasurement();
	bool lastLeadCompensation = false;
	int32_t compensationFrequency = 0;
	enum class State : uint8_t {
		Measuring,
		CompensationOpen,
		CompensationShort,
	};
	State state = State::Measuring;
	while (1) {
		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
		if (measurementUpdated) {
			ConfigureFrontendMeasurement();
			measurementUpdated = false;
			if (leadCompensation && compensationFrequency != measurementFrequency) {
				leadCompensation = false;
				lastLeadCompensation = false;
				Dialog::MessageBox("Warning", Font_Big,
						"Measurement frequency\nchanged. Lead\ncompensation has\nbeen disabled.", Dialog::MsgBox::OK,
						nullptr, false);
			}
		}
		if (newMeasurement) {
			LOG(Log_LCR, LevelDebug, "Got new measurement");
			switch (state) {
			case State::Measuring: {
				lastMeasurement = CalculateComponentValues(measurementResult);
				newMeasurement = false;

				const char *s = mainmenu->GetSelectedSubmenuName();
				static bool lastSweepActive = false;
				bool sweepActive = false;
				if (s && !strcmp(s, "Sweep")) {
					sweepActive = true;
				}

				if (sweepActive) {
					sweep->AddResult(lastMeasurement);
					Frontend::Start(sweep->GetAcquisitionSettings());
					sweep->requestRedraw();
				} else {
					cResult->requestRedraw();
				}

				if (sweepActive && !lastSweepActive) {
					sweep->setVisible(true);
					cResult->setVisible(false);
				} else if (!sweepActive && lastSweepActive) {
					sweep->setVisible(false);
					cResult->setVisible(true);
					ConfigureFrontendMeasurement();
					measurementUpdated = false;
				}

				lastSweepActive = sweepActive;
			}
				break;
			case State::CompensationOpen:
//				if (measurementResult.type != Frontend::ResultType::Valid) {
//					// ignore settling measurements
//					break;
//				}
				if(abs(measurementResult.Z) < 100000) {
					Dialog::MessageBox("Error", Font_Big, "\"Open\" measurement\nfailed.",
											Dialog::MsgBox::OK, nullptr, false);
					leadCompensation = false;
					lastLeadCompensation = false;
					state = State::Measuring;
					ConfigureFrontendMeasurement();
					break;
				}
				Frontend::Stop();
				if (Dialog::MessageBox("Lead compensation", Font_Big, "Compensation step 1/2:\nShort cables.",
						Dialog::MsgBox::ABORT_OK, nullptr, true) == Dialog::Result::OK) {
					state = State::CompensationShort;
					// Store result from open compensation
					Zopen = measurementResult.Z;
					// Start measurement with high averaging
					Frontend::Settings s;
					s.biasVoltage = biasVoltage;
					s.frequency = measurementFrequency;
					s.averages = 50;
					s.excitationVoltage = excitationVoltage;
					s.range = Frontend::Range::AUTO;
					Frontend::Start(s);
				} else {
					// user aborted
					leadCompensation = false;
					lastLeadCompensation = false;
					state = State::Measuring;
					mainmenu->requestRedrawChildren();
					ConfigureFrontendMeasurement();
				}
				break;
			case State::CompensationShort:
//				if (measurementResult.type != Frontend::ResultType::Valid) {
//					// ignore settling measurements
//					break;
//				}
				Zshort = measurementResult.Z;
				if (abs(Zshort) > 15) {
					Dialog::MessageBox("Error", Font_Big, "\"Short\" measurement\nfailed.", Dialog::MsgBox::OK, nullptr,
							false);
					leadCompensation = false;
					lastLeadCompensation = false;
					compensationFrequency = 0;
				} else {
					compensationFrequency = measurementFrequency;
				}
				state = State::Measuring;
				ConfigureFrontendMeasurement();
				break;
			}

			// trigger GUI task to redraw the result
			GUIEvent_t ev;
			ev.type = EVENT_NONE;
			GUI::SendEvent(&ev);
		}
		if (leadCompensation != lastLeadCompensation) {
			if (leadCompensation && compensationFrequency != measurementFrequency) {
				Frontend::Stop();
				if (Dialog::MessageBox("Lead compensation", Font_Big,
						"Compensation step 1/2:\nDisconnect cables\nfrom device.", Dialog::MsgBox::ABORT_OK, nullptr,
						true) == Dialog::Result::OK) {
					state = State::CompensationOpen;
					// Start measurement with high averaging
					Frontend::Settings s;
					s.biasVoltage = biasVoltage;
					s.frequency = measurementFrequency;
					s.averages = 50;
					s.excitationVoltage = excitationVoltage;
					s.range = Frontend::Range::AUTO;
					Frontend::Start(s);
				} else {
					// user aborted
					leadCompensation = false;
					mainmenu->requestRedrawChildren();
					ConfigureFrontendMeasurement();
				}
			}
			lastLeadCompensation = leadCompensation;
		}
	}
}
