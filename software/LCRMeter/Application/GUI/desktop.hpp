#pragma once

#include "widget.hpp"
#include "display.h"
#include "font.h"
#include "App.hpp"
#include <array>
#include "Unit.hpp"

class Desktop : public Widget {
public:
	Desktop();
	~Desktop();

	bool AddApp(App &app);
	bool FocusOnApp(App *app);
private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;
	void drawChildren(coords_t offset) override;

	bool WriteConfig();
	bool ReadConfig();

	Widget::Type getType() override { return Widget::Type::Desktop; };

	static constexpr color_t Foreground = COLOR_WHITE;
	static constexpr color_t Background = COLOR_BLACK;
	static constexpr uint8_t MaxApps = 6;
	static constexpr uint8_t IconBarWidth = 40;
	static constexpr uint8_t IconSpacing = 40;
	static constexpr uint8_t IconOffsetX = 4;
	static constexpr uint8_t IconOffsetY = 4;

	std::array<App*, MaxApps> apps;
	uint8_t AppCnt;
	int8_t focussed;
	uint32_t configIndex;
};

