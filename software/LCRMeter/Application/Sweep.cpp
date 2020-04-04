#include "Sweep.hpp"
#include "gui.hpp"
#include "HardwareLimits.hpp"
#include "log.h"

#define Log_Sweep (LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

static TaskHandle_t handle;

constexpr Sweep::Config Sweep::defaultConfig;
static constexpr char *variableNames[] = { "Disabled", "|Z|", "Phase",
		"Resistance", "Capacitance", "Inductance", "ESR", "Q-Factor",
		nullptr, };

Sweep* Sweep::Create(Config config) {
	handle = xTaskGetCurrentTaskHandle();
	auto callback_setTrueNotify = [](void *ptr, Widget*) {
		bool *flag = (bool*) ptr;
		*flag = true;
		if (handle) {
			xTaskNotify(handle, 0, eNoAction);
		}
	};

	bool Prev = false, Next = false, Abort = false, OK = false;
	bool YVarChanged = true; // force initial evalution of selected Y variables to enable/disable Y axis widgets
	enum class ScreenNumber : uint8_t {
		FrequencyAxis,
		PrimaryVariable,
		SecondaryVariable,
		AcquisitionSettings,
		Max
	};

	auto nscreen = ScreenNumber::FrequencyAxis;

	auto w = new Window("Sweep configuration", Font_Big, SIZE(DISPLAY_WIDTH, DISPLAY_HEIGHT));
	auto c = new Container(w->getAvailableArea());

	// context for radiobuttons on all screens
	Radiobutton::Set typeSet[(int) ScreenNumber::Max];
	memset(typeSet, 0, sizeof(typeSet));

	// navigational buttons
	constexpr uint16_t buttonClearance = 5;
	constexpr uint8_t buttonHeight = 30;
	auto buttonSize = SIZE((c->getSize().x - 4 * buttonClearance) / 3, buttonHeight);
	auto bPrev = new Button("Prev", Font_Big, callback_setTrueNotify, &Prev, buttonSize);
	auto bAbort = new Button("Abort", Font_Big, callback_setTrueNotify, &Abort, buttonSize);
	auto bNext = new Button("Next", Font_Big, callback_setTrueNotify, &Next, buttonSize);
	auto bOK = new Button("OK", Font_Big, callback_setTrueNotify, &OK, buttonSize);
	bPrev->setSelectable(false);
	bOK->setVisible(false);
	auto buttonY = c->getSize().y - buttonClearance - buttonSize.y;
	auto buttonPitchX = buttonClearance + buttonSize.x;
	c->attach(bPrev, COORDS(buttonClearance, buttonY));
	c->attach(bAbort, COORDS(buttonClearance + buttonPitchX, buttonY));
	c->attach(bOK, COORDS(buttonClearance + 2 * buttonPitchX, buttonY));
	c->attach(bNext, COORDS(buttonClearance + 2 * buttonPitchX, buttonY));

	Container *screens[(int) ScreenNumber::Max];
	for(uint8_t i=0;i<(int) ScreenNumber::Max;i++) {
		screens[i] = new Container(w->getAvailableArea() - SIZE(0, 2 * buttonClearance + buttonHeight));
		if (i != 0) {
			screens[i]->setVisible(false);
			c->attach(screens[i], COORDS(0, 0));
		}
	}
	c->attach(screens[0], COORDS(0, 0));
	// Elements of page 1: Frequency
	{
		auto lName = new Label(26, Font_Big, Label::Orientation::CENTER);
		lName->setText("X-Axis: Frequency");
		auto lXmin = new Label("Min:", Font_Big);
		auto lXmax = new Label("Max:", Font_Big);
		auto lPoints = new Label("Points:", Font_Big);
		auto eXmin = new Entry<uint32_t>(&config.X.f_min, HardwareLimits::MaxFrequency, HardwareLimits::MinFrequency,
				Font_Big, 9, Unit::Frequency);
		auto eXmax = new Entry<uint32_t>(&config.X.f_max, HardwareLimits::MaxFrequency, HardwareLimits::MinFrequency,
				Font_Big, 9, Unit::Frequency);
		auto ePoints = new Entry<uint16_t>(&config.X.points, 250, 2, Font_Big, 5, Unit::None);
		memset(&typeSet, 0, sizeof(typeSet));
		auto rLinear = new Radiobutton((uint8_t*) &config.X.type, 30, 0x00);
		auto lLinear = new Label("Linear", Font_Big);
		auto rLog = new Radiobutton((uint8_t*) &config.X.type, 30, 0x01);
		auto lLog = new Label("Log", Font_Big);
		rLinear->AddToSet(typeSet[0]);
		rLog->AddToSet(typeSet[0]);
		// attach to x axis container
		screens[0]->attach(lName, COORDS(4, 2));
		screens[0]->attach(lXmin, COORDS(50, 40));
		screens[0]->attach(eXmin, COORDS(120, 38));
		screens[0]->attach(lXmax, COORDS(50, 70));
		screens[0]->attach(eXmax, COORDS(120, 68));
		screens[0]->attach(lPoints, COORDS(50, 100));
		screens[0]->attach(ePoints, COORDS(168, 98));
		screens[0]->attach(rLinear, COORDS(40, 140));
		screens[0]->attach(lLinear, COORDS(75, 147));
		screens[0]->attach(rLog, COORDS(160, 140));
		screens[0]->attach(lLog, COORDS(195, 147));
	}

	// Elements for primary and secondary Y axis

	// These elements have to be accessible later, store in pointer
	Entry<float> *emin[2];
	Entry<float> *emax[2];
	Radiobutton *rLog[2];
	Radiobutton *rLinear[2];

	for (uint8_t i = 0; i < 2; i++) {
		auto lName = new Label(26, Font_Big, Label::Orientation::CENTER);
		if(i==0) {
			lName->setText("Primary Y axis");
		} else {
			lName->setText("Secondary Y axis");
		}
		auto cVar = new ItemChooser(variableNames, (uint8_t*) &config.axis[i].var,
				Font_Big, 9, 0);
		cVar->setCallback(callback_setTrueNotify, &YVarChanged);
		auto lmin = new Label("Min:", Font_Big);
		auto lmax = new Label("Max:", Font_Big);
		emin[i] = new Entry<float>(&config.axis[i].min, nullptr, nullptr, Font_Big, 7, Unit::Frequency);
		emax[i] = new Entry<float>(&config.axis[i].max, nullptr, nullptr, Font_Big, 7, Unit::Frequency);
		rLinear[i] = new Radiobutton((uint8_t*) &config.axis[i].type, 30, 0x00);
		auto lLinear = new Label("Linear", Font_Big);
		rLog[i] = new Radiobutton((uint8_t*) &config.axis[i].type, 30, 0x01);
		auto lLog = new Label("Log", Font_Big);
		rLinear[i]->AddToSet(typeSet[i + 1]);
		rLog[i]->AddToSet(typeSet[i + 1]);
		screens[i+1]->attach(lName, COORDS(4, 2));
		screens[i+1]->attach(cVar, COORDS(5, 25));
		screens[i+1]->attach(lmin, COORDS(170, 40));
		screens[i+1]->attach(emin[i], COORDS(220, 38));
		screens[i+1]->attach(lmax, COORDS(170, 70));
		screens[i+1]->attach(emax[i], COORDS(220, 68));
		screens[i+1]->attach(rLinear[i], COORDS(170, 100));
		screens[i+1]->attach(lLinear, COORDS(205, 107));
		screens[i+1]->attach(rLog[i], COORDS(170, 140));
		screens[i+1]->attach(lLog, COORDS(205, 147));
	}

	// Acquisition widgets
	{
		auto lName = new Label(26, Font_Big, Label::Orientation::CENTER);
		lName->setText("Acquisition Settings");
		auto lAvg = new Label("Averages:", Font_Big);
		auto lExcitation = new Label("Excitation:", Font_Big);
		auto lBias = new Label("Bias Voltage:", Font_Big);
		auto eAvg = new Entry<uint32_t>(&config.averages, 100, 1, Font_Big, 8,
				Unit::None);
		auto eExcitation = new Entry<uint32_t>(&config.excitationVoltage,
				HardwareLimits::MaxExcitationVoltage,
				HardwareLimits::MinExcitationVoltage, Font_Big, 8,
				Unit::Voltage);
		auto eBias = new Entry<uint32_t>(&config.biasVoltage,
				HardwareLimits::MaxBiasVoltage, HardwareLimits::MinBiasVoltage,
				Font_Big, 8, Unit::Voltage);
		screens[3]->attach(lName, COORDS(4, 2));
		screens[3]->attach(lAvg, COORDS(30, 40));
		screens[3]->attach(eAvg, COORDS(190, 38));
		screens[3]->attach(lExcitation, COORDS(30, 70));
		screens[3]->attach(eExcitation, COORDS(190, 68));
		screens[3]->attach(lBias, COORDS(30, 100));
		screens[3]->attach(eBias, COORDS(190, 98));
	}

	w->setMainWidget(c);

	while (!Abort && !OK) {
		if (Prev) {
			Prev = false;
			if (nscreen != ScreenNumber::FrequencyAxis) {
				nscreen = (ScreenNumber) ((int) nscreen - 1);
			}
			if (nscreen == ScreenNumber::FrequencyAxis) {
				// first screen, disable "previous" button
				bPrev->setSelectable(false);
			}
			bOK->setVisible(false);
			bNext->setVisible(true);
		}
		if (Next) {
			Next = false;
			if (nscreen != (ScreenNumber) ((int) ScreenNumber::Max - 1)) {
				nscreen = (ScreenNumber) ((int) nscreen + 1);
			}
			if (nscreen == (ScreenNumber) ((int) ScreenNumber::Max - 1)) {
				// last screen, show "start" instead of "next"
				bOK->setVisible(true);
				bNext->setVisible(false);
			}
			bPrev->setSelectable(true);
		}
		for (uint8_t i = 0; i < (int) ScreenNumber::Max; i++) {
			if (i == (int) nscreen) {
				screens[i]->setVisible(true);
			} else {
				screens[i]->setVisible(false);
			}
		}
		if (YVarChanged) {
			YVarChanged = false;
			for (uint8_t i = 0; i < 2; i++) {
				bool selectable = config.axis[i].var != Variable::None;
				emin[i]->setSelectable(selectable);
				emax[i]->setSelectable(selectable);
				rLog[i]->setSelectable(selectable);
				rLinear[i]->setSelectable(selectable);
			}
		}
		xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY);
	}
	delete w;
	if(OK) {
		auto s = new Sweep(config);
		return s;
	} else {
		return nullptr;
	}
}

using Datapoint = struct {
	float primary;
	float secondary;
};

bool Sweep::Start() {
	pointCnt = 0;
	if (points) {
		delete points;
	}
	// Allocate memory for sweep results
	points = new Datapoint[config.X.points];
	if (!points) {
		return false;
	}
	return true;
}

Frontend::settings Sweep::GetAcquisitionSettings() {
	Frontend::settings s;
	s.biasVoltage = config.biasVoltage;
	s.excitationVoltage = config.excitationVoltage;
	s.averages = config.averages;
	s.range = config.range;
	switch(config.X.type) {
	case ScaleType::Linear:
		s.frequency = util_Map(pointCnt, 0, config.X.points - 1, config.X.f_min,
				config.X.f_max);
		break;
	case ScaleType::Log: {
		float b = log(config.X.f_max/config.X.f_min) / (config.X.points - 1);
		s.frequency = config.X.f_min * exp(b * pointCnt);
	}
		break;
	}
	return s;
}

bool Sweep::AddResult(LCR::Result r) {
	if (pointCnt >= config.X.points) {
		LOG(Log_Sweep, LevelWarn, "Unable to add point, already at maximum");
		return false;
	}
	if (!points) {
		LOG(Log_Sweep, LevelWarn, "Unable to add point, no memory");
		return false;
	}
	// extract the correct variables
	float vars[2];
	for(uint8_t i=0;i<2;i++) {
		switch(config.axis[i].var) {
		case Variable::Magnitude:
			vars[i] = abs(r.frontend.Z);
			break;
		case Variable::Phase:
			vars[i] = 180.0f / M_PI * arg(r.frontend.Z);
			break;
		case Variable::Resistance:
			vars[i] = real(r.Z);
			break;
		case Variable::Capacitance:
			vars[i] = r.C.capacitance;
			break;
		case Variable::Inductance:
			vars[i] = r.L.inductance;
			break;
		case Variable::ESR:
			vars[i] = real(r.frontend.Z);
			break;
		case Variable::Quality:
			vars[i] = r.qualityFactor;
			break;
		default:
			vars[i] = 0.0f;
		}
	}
	points[pointCnt].Primary = vars[0];
	points[pointCnt].Secondary = vars[1];
	pointCnt++;
	LOG(Log_Sweep, LevelDebug, "Added datapoint %d", pointCnt);
	return true;
}

Sweep::~Sweep() {
	if(points) {
		delete points;
	}
}

bool Sweep::Done() {
	if (pointCnt >= config.X.points) {
		return true;
	} else {
		return false;
	}
}

Sweep::Sweep(Config c) {
	config = c;
	pointCnt = 0;
	points = 0;
}

void Sweep::Display(coords_t pos, coords_t size, bool onlyData) {
	coords_t graphTopLeft = pos + COORDS(Font_Medium.height + 2, 0);
	coords_t graphBottomRight = pos + size - COORDS(Font_Medium.height + 2, Font_Medium.height + 2);
	if (!onlyData) {
		// fill background
		display_SetForeground(ColorBackground);
		display_SetBackground(ColorBackground);
		display_RectangleFull(pos.x, pos.y, pos.x + size.x - 1, pos.y + size.y - 1);
		// draw X axis
		display_SetForeground(ColorAxis);
		display_HorizontalLine(graphTopLeft.x, graphBottomRight.y, graphBottomRight.x - graphTopLeft.x + 1);
		// extreme ticks for X axis
		char tick[6];
		Unit::SIStringFromFloat(tick, 5, config.X.f_min);
		display_SetFont(Font_Medium);
		display_String(pos.x, pos.y + size.y - Font_Medium.height, tick);
		Unit::SIStringFromFloat(tick, 5, config.X.f_max);
		display_String(pos.x + size.x - strlen(tick) * Font_Medium.width, pos.y + size.y - Font_Medium.height, tick);
		const char *xlabel = "Frequency";
		display_String((pos.x + size.x - strlen(xlabel) * Font_Medium.width) / 2, pos.y + size.y - Font_Medium.height,
				xlabel);

		// extreme ticks and label for primary Y axis
		if (config.axis[0].var != Variable::None) {
			display_SetForeground(ColorPrimary);
			display_VerticalLine(graphTopLeft.x, graphTopLeft.y, graphBottomRight.y - graphTopLeft.y);
			Unit::SIStringFromFloat(tick, 5, config.axis[0].min);
			display_StringRotated(pos.x + 1, graphBottomRight.y, tick);
			Unit::SIStringFromFloat(tick, 5, config.axis[0].max);
			display_StringRotated(pos.x + 1, pos.y + strlen(tick) * Font_Medium.width, tick);
			// Label
			const char *label = variableNames[(int) config.axis[0].var];
			display_StringRotated(pos.x + 1, (pos.y + graphBottomRight.y + strlen(label) * Font_Medium.width) / 2, label);
		}

		// extreme ticks and label for secondary Y axis
		if (config.axis[1].var != Variable::None) {
			display_SetForeground(ColorSecondary);
			display_VerticalLine(graphBottomRight.x, graphTopLeft.y, graphBottomRight.y - graphTopLeft.y);
			Unit::SIStringFromFloat(tick, 5, config.axis[1].min);
			display_StringRotated(pos.x + size.x - Font_Medium.height, graphBottomRight.y, tick);
			Unit::SIStringFromFloat(tick, 5, config.axis[1].max);
			display_StringRotated(pos.x + size.x - Font_Medium.height, pos.y + strlen(tick) * Font_Medium.width, tick);
			// Label
			const char *label = variableNames[(int) config.axis[1].var];
			display_StringRotated(pos.x + size.x - Font_Medium.height,
					(pos.y + graphBottomRight.y + strlen(label) * Font_Medium.width) / 2, label);
		}
	}
	// display data points
	for (uint8_t axis = 0; axis < 2; axis++) {
		if (config.axis[axis].var == Variable::None) {
			// this axis is not active
			continue;
		}
		if (axis == 0) {
			display_SetForeground(ColorPrimary);
		} else {
			display_SetForeground(ColorSecondary);
		}
		float a = 0, b = 0;
		if (config.axis[axis].type == ScaleType::Log) {
			b = log(config.axis[axis].max / config.axis[axis].min) / (graphTopLeft.y - graphBottomRight.y);
			a = config.axis[axis].max / exp(b * graphTopLeft.y);
		}
		for (uint16_t i = 1; i < pointCnt; i++) {
			float now, last;
			if (axis == 0) {
				now = points[i].Primary;
				last = points[i - 1].Primary;
			} else {
				now = points[i].Secondary;
				last = points[i - 1].Secondary;
			}
			// constrain value to limits
			if (now < config.axis[axis].min) {
				now = config.axis[axis].min;
			}
			if (now > config.axis[axis].max) {
				now = config.axis[axis].max;
			}
			if (last < config.axis[axis].min) {
				last = config.axis[axis].min;
			}
			if (last > config.axis[axis].max) {
				last = config.axis[axis].max;
			}
			int16_t x0, y0, x1, y1;
			x0 = util_Map(i-1, 0, config.X.points - 1, graphTopLeft.x, graphBottomRight.x);
			x1 = util_Map(i, 0, config.X.points - 1, graphTopLeft.x, graphBottomRight.x);
			if (config.axis[axis].type == ScaleType::Linear) {
				y0 = util_MapF(last, config.axis[axis].min, config.axis[axis].max, graphBottomRight.y,
						graphTopLeft.y);
				y1 = util_MapF(now, config.axis[axis].min, config.axis[axis].max, graphBottomRight.y,
						graphTopLeft.y);
			} else {
				y0 = log(last / a) / b;
				y1 = log(now / a) / b;
			}
			display_Line(x0, y0, x1, y1);
		}
	}
}
