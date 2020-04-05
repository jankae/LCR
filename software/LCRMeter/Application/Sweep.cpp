#include "Sweep.hpp"
#include "gui.hpp"
#include "HardwareLimits.hpp"
#include "log.h"
#include "cast.hpp"

#define Log_Sweep (LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

constexpr Sweep::Config Sweep::defaultConfig;
static constexpr char *variableNames[] = { "Disabled", "|Z|", "Phase",
		"Resistance", "Capacitance", "Inductance", "ESR", "Q-Factor",
		nullptr, };
static constexpr char *scaleTypeNames[] = { "Linear", "Log", nullptr };


Sweep::Sweep(coords_t size, Menu &menu, Config c) {
	this->size = size;
	config = c;
	initialSweep = true;
	pointCnt = 0;
	// Create menu entries
	mConfig = new Menu("Sweep", menu.getSize());
	// X axis menu
	auto mX = new Menu("Frequency\nSetup", menu.getSize());
	auto mXmin = new MenuValue<uint32_t>("Min.Freq", &config.X.f_min, Unit::Frequency,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MayorSettingChanged>::cfn, this,
			HardwareLimits::MinFrequency, HardwareLimits::MaxFrequency);
	auto mXmax = new MenuValue<uint32_t>("Max.Freq", &config.X.f_max, Unit::Frequency,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MayorSettingChanged>::cfn, this,
			HardwareLimits::MinFrequency, HardwareLimits::MaxFrequency);
	auto mPoints = new MenuValue<uint16_t>("Points", &config.X.points, Unit::None,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MayorSettingChanged>::cfn, this, 2, MaxDataPoints);
	auto mXScale = new MenuChooser("Scale", scaleTypeNames, (uint8_t*) &config.X.type,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MayorSettingChanged>::cfn, this, false);
	mX->AddEntry(mXmin);
	mX->AddEntry(mXmax);
	mX->AddEntry(mPoints);
	mX->AddEntry(mXScale);
	mX->AddEntry(new MenuBack());
	// Primary and secondary Y axis menu
	Menu *mAxis[2];
	for (uint8_t i = 0; i < 2; i++) {
		auto mVar = new MenuChooser("Variable", variableNames, (uint8_t*) &config.axis[i].var,
				pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MayorSettingChanged>::cfn, this);
		auto mMin = new MenuValue<float>("Y min", &config.axis[i].min, Unit::None,
				pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MinorSettingChanged>::cfn, this);
		auto mMax = new MenuValue<float>("Y max", &config.axis[i].max, Unit::None,
				pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MinorSettingChanged>::cfn, this);
		auto mScale = new MenuChooser("Scale", scaleTypeNames, (uint8_t*) &config.axis[i].type,
				pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MinorSettingChanged>::cfn, this, false);
		constexpr char *MenuNames[] = { "Primary\nY-axis", "Secondary\nY-axis" };
		mAxis[i] = new Menu(MenuNames[i], menu.getSize());
		mAxis[i]->AddEntry(mVar);
		mAxis[i]->AddEntry(mMin);
		mAxis[i]->AddEntry(mMax);
		mAxis[i]->AddEntry(mScale);
		mAxis[i]->AddEntry(new MenuBack());
	}
	// acquisition menu
	auto mAcq = new Menu("Acquisition\nSettings", menu.getSize());
	auto mAvg = new MenuValue<uint16_t>("Averages", &config.averages, Unit::None,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MinorSettingChanged>::cfn, this, 1, 1000);
	auto mExc = new MenuValue<uint32_t>("Excitation", &config.excitationVoltage, Unit::Voltage,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MinorSettingChanged>::cfn, this,
			HardwareLimits::MinExcitationVoltage, HardwareLimits::MaxExcitationVoltage);
	auto mBias = new MenuValue<uint32_t>("Bias", &config.biasVoltage, Unit::Voltage,
			pmf_cast<void (*)(void*, Widget *w), Sweep, &Sweep::MinorSettingChanged>::cfn, this,
			HardwareLimits::MinBiasVoltage, HardwareLimits::MaxBiasVoltage);
	mAcq->AddEntry(mAvg);
	mAcq->AddEntry(mExc);
	mAcq->AddEntry(mBias);
	mAcq->AddEntry(new MenuBack());

	// Add menus to main config menu
	mConfig->AddEntry(mX);
	mConfig->AddEntry(mAxis[0]);
	mConfig->AddEntry(mAxis[1]);
	mConfig->AddEntry(mAcq);
	mConfig->AddEntry(new MenuBack);

	// Add config menu to main menu
	menu.AddEntry(mConfig);
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
		// wrap around to beginning
		pointCnt = 0;
		initialSweep = false;
	}
	if (!points) {
		LOG(Log_Sweep, LevelWarn, "Unable to add point, no memory");
		return false;
	}
	// extract the correct variables
	for(uint8_t i=0;i<2;i++) {
		float var;
		switch(config.axis[i].var) {
		case Variable::Magnitude:
			var = abs(r.frontend.Z);
			break;
		case Variable::Phase:
			var = 180.0f / M_PI * arg(r.frontend.Z);
			break;
		case Variable::Resistance:
			var = real(r.Z);
			break;
		case Variable::Capacitance:
			var = r.C.capacitance;
			break;
		case Variable::Inductance:
			var = r.L.inductance;
			break;
		case Variable::ESR:
			var = real(r.frontend.Z);
			break;
		case Variable::Quality:
			var = r.qualityFactor;
			break;
		default:
			var = 0.0f;
		}
		points[pointCnt].y[i] = var;
	}
	pointCnt++;
	LOG(Log_Sweep, LevelDebug, "Added datapoint %d", pointCnt);
	return true;
}

void Sweep::draw(coords_t offset) {
	size = getSize();
	auto pos = offset;
	coords_t graphTopLeft = pos + COORDS(Font_Medium.height + 2, 0);
	coords_t graphBottomRight = pos + size - COORDS(Font_Medium.height + 2, Font_Medium.height + 2);

	auto GetPointCoordinate = [this, graphTopLeft, graphBottomRight](uint8_t axis, uint16_t point) -> coords_t {
		coords_t p;
		float val = points[point].y[axis];
		// constrain value to limits
		if (val < config.axis[axis].min) {
			val = config.axis[axis].min;
		}
		if (val > config.axis[axis].max) {
			val = config.axis[axis].max;
		}
		p.x = util_Map(point, 0, config.X.points - 1, graphTopLeft.x + 1, graphBottomRight.x - 1);
		if (config.axis[axis].type == ScaleType::Linear) {
			p.y = util_MapF(val, config.axis[axis].min, config.axis[axis].max, graphBottomRight.y - 1,
					graphTopLeft.y + 1);
		} else {
			float b = log(config.axis[axis].max / config.axis[axis].min)
					/ (graphTopLeft.y + 1 - graphBottomRight.y - 1);
			float a = config.axis[axis].max / exp(b * (graphTopLeft.y + 1));
			p.y = log(val / a) / b;
		}
		return p;
	};

	if (redrawClear) {
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
		const char *xlabel = config.X.type == ScaleType::Linear ? "Frequency (linear)" : "Frequency (log)";
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
			char label[50];
			strcpy(label, variableNames[(int) config.axis[0].var]);
			strcat(label, config.axis[0].type == ScaleType::Linear ? " (linear)" : " (log)");
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
			char label[50];
			strcpy(label, variableNames[(int) config.axis[1].var]);
			strcat(label, config.axis[1].type == ScaleType::Linear ? " (linear)" : " (log)");
			display_StringRotated(pos.x + size.x - Font_Medium.height,
					(pos.y + graphBottomRight.y + strlen(label) * Font_Medium.width) / 2, label);
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
			uint16_t highestPoint = initialSweep ? pointCnt : config.X.points;
			for (uint16_t i = 1; i < highestPoint; i++) {
				coords_t from = GetPointCoordinate(axis, i - 1);
				coords_t to = GetPointCoordinate(axis, i);
				display_Line(from.x, from.y, to.x, to.y);
			}
		}
	} else {
		// only update latest datapoint
		if (pointCnt > 1) {
			bool cleared = false;
			for (uint8_t axis = 0; axis < 2; axis++) {
				if (config.axis[axis].var == Variable::None) {
					// this axis is not active
					continue;
				}
				coords_t from = GetPointCoordinate(axis, pointCnt - 2);
				coords_t to = GetPointCoordinate(axis, pointCnt - 1);
				if (!cleared) {
					display_SetForeground(ColorBackground);
					uint16_t x1 = GetPointCoordinate(axis, pointCnt).x;
					if (x1 - from.x < 5) {
						x1 = from.x + 5;
					}
					if (x1 >= graphBottomRight.x) {
						x1 = graphBottomRight.x - 1;
					}
					display_RectangleFull(from.x + 1, graphTopLeft.y + 1, x1, graphBottomRight.y - 1);
					cleared = true;
				}
				if (axis == 0) {
					display_SetForeground(ColorPrimary);
				} else {
					display_SetForeground(ColorSecondary);
				}
				display_Line(from.x, from.y, to.x, to.y);
			}
		}
	}
}

void Sweep::MayorSettingChanged(Widget *w) {
	initialSweep = true;
	pointCnt = 0;
	// TODO check settings
	requestRedrawFull();
}

void Sweep::MinorSettingChanged(Widget *w) {
	// TODO check settings
	requestRedrawFull();
}

Sweep::~Sweep() {
	if (mConfig) {
		delete mConfig;
	}
}
