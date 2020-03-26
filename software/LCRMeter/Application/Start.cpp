#include "Start.h"
#include "log.h"
#include <cstring>
#include "display.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "input.h"
#include "desktop.hpp"
#include "gui.hpp"
#include "touch.h"
#include "Config.hpp"
#include "Persistence.hpp"
#include "LCR.hpp"
#include "Frontend.hpp"

extern ADC_HandleTypeDef hadc1;
// global mutex controlling access to SPI1 (used for touch + SD card)
SemaphoreHandle_t xMutexSPI1;
static StaticSemaphore_t xSemSPI1;

static bool VCCRail() {
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADC_Start(&hadc1);
	if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
		return false;
	}
	uint16_t raw = HAL_ADC_GetValue(&hadc1);
	constexpr float ReferenceTypical = 1.2f;
	constexpr float ExpectedSupply = 3.3f;
	constexpr uint16_t ExpectedADC = ReferenceTypical * 4096 / ExpectedSupply;
	uint16_t supply = 3300 * ExpectedADC / raw;
	LOG(Log_App, LevelInfo, "Supply voltage: %dmV", supply);
	if(supply > 3100 && supply < 3500) {
		return true;
	} else {
		return false;
	}
}

using Test = struct {
	const char *name;
	bool (*function)(void);
};

constexpr Test Selftests[] = {
		{"3V3 rail", VCCRail},
		{"Frontend init", Frontend::Init},
		{"Touch thread:", input_Init},
		{"Persistance:", Persistence::Load},
};
constexpr uint8_t nTests = sizeof(Selftests) / sizeof(Selftests[0]);


void Start() {
	log_init();
	LOG(Log_App, LevelInfo, "Start");
	Persistence::Init();
	touch_Init();

	xMutexSPI1 = xSemaphoreCreateMutexStatic(&xSemSPI1);

	// initialize display
	//vTaskDelay(1);
	display_SetBackground(COLOR_BLACK);
	display_SetForeground(COLOR_WHITE);
	display_SetFont(Font_Big);
	display_Clear();
	display_String(0, 0, "Running selftest...");

	bool passed = true;
	const uint8_t fontheight = Font_Big.height;
	const uint8_t fontwidth = Font_Big.width;
	for (uint8_t i = 0; i < nTests; i++) {
		display_String(0, fontheight * (i + 1), Selftests[i].name);
		bool result = Selftests[i].function();
		if (!result) {
			passed = false;
			display_SetForeground(COLOR_RED);
			display_String(DISPLAY_WIDTH - 1 - 6 * fontwidth,
					fontheight * (i + 1), "FAILED");
			LOG(Log_App, LevelError, "Failed selftest: %s", Selftests[i].name);
		} else {
			display_SetForeground(COLOR_GREEN);
			display_String(DISPLAY_WIDTH - 1 - 6 * fontwidth,
					fontheight * (i + 1), "PASSED");
			LOG(Log_App, LevelInfo, "Passed selftest: %s", Selftests[i].name);
		}
		display_SetForeground(COLOR_WHITE);
	}

	display_String(0, DISPLAY_HEIGHT - Font_Big.height - 1,
			"Press screen to continue");
	{
		coords_t dummy;
		while (!touch_GetCoordinates(&dummy))
			;
		while (touch_GetCoordinates(&dummy))
			;
	}

//	Loadcells::Setup(0x3F, MAX11254_RATE_CONT1_9_SINGLE50);
//	Input::Init();
//	Input::Calibrate();
//	Desktop d;
//	App::Info app;
//	app.task = LoadcellSetup::Task;
//	app.StackSize = 512;
//	app.name = "Loadcell setup";
//	app.descr = "Calibrate and configure loadcells";
//	app.icon = &LoadcellSetup::Icon;
//	new App(app, d);
//
//	App::Info app2;
//	app2.task = DriverControl::Task;
//	app2.StackSize = 1024;
//	app2.name = "Motor driver";
//	app2.descr = "Setup and control the motor";
//	app2.icon = &DriverControl::Icon;
//	new App(app2, d);
//
//	App::Info app3;
//	app3.task = Setup::Task;
//	app3.StackSize = 768;
//	app3.name = "Setup";
//	app3.descr = "Configure device settings";
//	app3.icon = &Setup::Icon;
//	new App(app3, d);

//	GUI::Init(d);

//	Config::Load("default.cfg");
	LCR::Init();

	LCR::Run(); // does not return
	while(1) {
		vTaskDelay(1000);
	}
}
