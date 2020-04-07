#pragma once

#include <stdint.h>
#include "Frontend.hpp"
#include "LCR.hpp"
#include "widget.hpp"
#include "menu.hpp"

class Sweep : public Widget {
public:
	enum class ScaleType : uint8_t {
		Linear = 0x00,
		Log = 0x01,
	};
	enum class Variable : uint8_t {
		None = 0x00,
		Magnitude = 0x01,
		Phase = 0x02,
		Resistance = 0x03,
		Capacitance = 0x04,
		Inductance = 0x05,
		ESR = 0x06,
		Quality = 0x07,
	};
	using YAxis = struct _axis {
		float min;
		float max;
		ScaleType type;
		Variable var;
	};
	using Config = struct _config {
		struct {
			uint32_t f_min;
			uint32_t f_max;
			ScaleType type;
			uint16_t points;
		} X;
		YAxis axis[2];
		uint32_t biasVoltage;
		uint32_t excitationVoltage;
		Frontend::Range range;
		uint16_t averages;
	};
	static constexpr Config defaultConfig = {
			.X = {.f_min = 100, .f_max = 100000, .type = ScaleType::Linear, .points=101},
			.axis = {
					{.min = 0.0f, .max = 10.0f, .type = ScaleType::Linear, .var = Variable::Magnitude},
					{.min = 0.0f, .max = 10.0f, .type = ScaleType::Linear, .var = Variable::ESR},
			},
			.biasVoltage = 0,
			.excitationVoltage = 100000,
			.range = Frontend::Range::AUTO,
			.averages = 1,
	};

	Sweep(coords_t size, Menu &menu, Config c = defaultConfig);
	~Sweep();
	Frontend::settings GetAcquisitionSettings();
	bool AddResult(LCR::Result r);
private:
	static constexpr color_t ColorBackground = COLOR_BG_DEFAULT;
	static constexpr color_t ColorAxis = COLOR_BLACK;
	static constexpr color_t ColorPrimary = COLOR_DARKGREEN;
	static constexpr color_t ColorSecondary = COLOR_RED;
	static constexpr color_t ColorMarker = COLOR_LIGHTGRAY;
	static constexpr uint16_t MaxDataPoints = 250;

	using Datapoint = struct {
		float y[2];
	};

	Widget::Type getType() override { return Widget::Type::Custom; };

	// Called whenever a setting has changed that requires the sweep to reset
	void MayorSettingChanged(Widget *w);
	// Called whenever a setting has changed that only influences the sweep display (e.i. Y axis scaling)
	void MinorSettingChanged(Widget *w);

	uint32_t PointToFrequency(uint16_t point);

	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

//	Sweep(Config c);

	Menu *mConfig;
	Config config;
	Datapoint points[MaxDataPoints];
	uint16_t pointCnt;
	bool initialSweep;
	uint16_t marker;
};
