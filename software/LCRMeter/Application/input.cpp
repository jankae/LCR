#include <util.h>
#include "input.hpp"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "touch.hpp"
#include "log.h"
#include "fatfs.h"
#include "file.hpp"
#include "display.h"

#include "gui.hpp"

// TODO remove hardcoded value, save to file instead
static int32_t offsetX = -30;
static int32_t offsetY = -27;
static float scaleX = 0.0932;//(float) DISPLAY_WIDTH / 4096;
static float scaleY = 0.0677;//(float) DISPLAY_HEIGHT / 4096;

static constexpr File::Entry touchCal[4] = {
		{"xfactor", &scaleX, File::PointerType::FLOAT},
		{"xoffset", &offsetX, File::PointerType::INT32},
		{"yfactor", &scaleY, File::PointerType::FLOAT},
		{"yoffset", &offsetY, File::PointerType::INT32},
};

static void penirq(void *ptr) {
	TaskHandle_t h = (TaskHandle_t) ptr;
	BaseType_t yield = pdFALSE;
	xTaskNotifyFromISR(h, 0, eNoAction, &yield);
	portYIELD_FROM_ISR(yield);
}

static void inputThread(void *ptr) {
	using namespace Input;

	coords_t lastTouch;
	coords_t initialTouch;
	uint32_t touchStart = 0;
	bool touchMoved = false;
	bool touched = false;
	bool longTouch = false;
	LOG(Log_Input, LevelInfo, "Thread start");

	uint32_t lastDragEvent = 0;

	Touch::SetPENCallback(penirq, xTaskGetCurrentTaskHandle());
	while (1) {
		coords_t touch;
		lastTouch = touch;
		uint32_t wait = portMAX_DELAY;
		if (touched) {
			wait = pdMS_TO_TICKS(20);
		}
		xTaskNotifyWait(0, 0, NULL, wait);
		bool old = touched;
		touched = Touch::GetCoordinates(touch);
		if (touched) {
			// clear additional notification during sampling
			xTaskNotifyWait(0, 0, NULL, 0);
		}
		touch.x = constrain_int16_t(touch.x * scaleX + offsetX, 0,
				DISPLAY_WIDTH - 1);
		touch.y = constrain_int16_t(touch.y * scaleY + offsetY, 0,
				DISPLAY_HEIGHT - 1);
		GUIEvent_t ev;
		ev.type = EVENT_NONE;
		if (!old && touched) {
			LOG(Log_Input, LevelDebug, "Touch pressed: %d, %d", touch.x,
					touch.y);
			initialTouch = touch;
			longTouch = false;
			touchMoved = false;
			touchStart = HAL_GetTick();
			lastDragEvent = touchStart;
			ev.type = EVENT_TOUCH_PRESSED;
			ev.pos = touch;
		} else if(old && !touched) {
			LOG(Log_Input, LevelDebug, "Touch released", lastTouch.x,
					lastTouch.y);
			ev.type = EVENT_TOUCH_RELEASED;
			ev.pos = lastTouch;
		} else if(old && touched) {
			// check if continually pressed for some time
			if (!longTouch && !touchMoved
					&& HAL_GetTick() - touchStart > LongTouchTime) {
				LOG(Log_Input, LevelDebug, "Long touch: %d, %d",
						initialTouch.x, initialTouch.y);
				longTouch = true;
				ev.type = EVENT_TOUCH_HELD;
				ev.pos = initialTouch;
			} else if (!longTouch) {
				if (touchMoved || abs(initialTouch.x - touch.x) > 20
						|| abs(initialTouch.y - touch.y) > 20) {
					if (HAL_GetTick() - lastDragEvent > 250) {
						lastDragEvent = HAL_GetTick();
						LOG(Log_Input, LevelDebug, "touch dragged");
						touchMoved = true;
						ev.type = EVENT_TOUCH_DRAGGED;
						ev.pos = initialTouch;
						ev.dragged = touch;
					}
				}
			}
		}

		if (ev.type != EVENT_NONE) {
			if (!GUI::SendEvent(&ev)) {
				LOG(Log_Input, LevelWarn, "Event queue full");
			}
		}
	}
}

bool Input::Init() {
	xTaskCreate(inputThread, "INPUT", 256, nullptr, 5, nullptr);
	return true;
}

static coords_t GetCalibrationPoint(bool top, coords_t cross) {
	constexpr uint16_t barHeight = 20;
	int16_t y = top ? 0 : DISPLAY_HEIGHT - Font_Big.height - barHeight - 1;
	display_SetForeground(COLOR_WHITE);
	display_SetBackground(COLOR_BLACK);
	display_Clear();
	display_Line(cross.x - 10, cross.y - 10, cross.x + 10, cross.y + 10);
	display_Line(cross.x - 10, cross.y + 10, cross.x + 10, cross.y - 10);
	display_SetFont(Font_Big);
	display_String(0, y, "Press and hold X");
	display_Rectangle(0, y + Font_Big.height, DISPLAY_WIDTH - 1, y + Font_Big.height + barHeight);
	uint16_t bar = 1;
	uint16_t samples = 0;
	int32_t sumX = 0, sumY = 0;
	coords_t c;
	do {
		if(Touch::GetCoordinates(c)) {
			bar++;
			if(bar >= 50) {
				sumX += c.x;
				sumY += c.y;
				samples++;
			}
			display_SetForeground(COLOR_GREEN);
			display_VerticalLine(bar, y + Font_Big.height + 1, barHeight - 2);
		} else {
			bar = 1;
			samples = 0;
			sumX = sumY = 0;
			display_SetForeground(COLOR_BLACK);
			display_RectangleFull(1, y + Font_Big.height + 1, DISPLAY_WIDTH - 2,
					y + Font_Big.height + barHeight - 1);
		}
	} while (bar < DISPLAY_WIDTH - 2);
	// wait for touch to be released
	while (Touch::GetCoordinates(c))
		;
	coords_t ret;
	ret.x = sumX / samples;
	ret.y = sumY / samples;
	return ret;
}

static bool SaveCalibration(void) {
	if (File::Open("TOUCH.CAL", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
		LOG(Log_Input, LevelError, "Failed to create calibration file");
		return false;
	}
	File::WriteParameters(touchCal, 4);
	File::Close();
	LOG(Log_Input, LevelInfo, "Created calibration file");
	return true;
}


void Input::Calibrate() {
	void *ptr;
	exti_callback_t cb;
	exti_get_callback(TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin, &cb, &ptr);
	Touch::ClearPENCallback();
	coords_t Set1 = { .x = 20, .y = 20 };
	coords_t Set2 = { .x = DISPLAY_WIDTH - 20, .y = DISPLAY_HEIGHT - 20 };
	coords_t Meas1 = GetCalibrationPoint(false, Set1);
	coords_t Meas2 = GetCalibrationPoint(true, Set2);

	scaleX = (float) (Set2.x - Set1.x) / (Meas2.x - Meas1.x);
	scaleY = (float) (Set2.y - Set1.y) / (Meas2.y - Meas1.y);
	/* calculate offset */
	offsetX = Set1.x - Meas1.x * scaleX;
	offsetY = Set1.y - Meas1.y * scaleY;

	GUIEvent_t ev;
	ev.type = EVENT_WINDOW_CLOSE;
	GUI::SendEvent(&ev);

	if(!SaveCalibration()) {
		Dialog::MessageBox("ERROR", Font_Big,
				"Failed to save\ntouch calibration", Dialog::MsgBox::OK, nullptr,
				true);
	}

	// restore exti callback to input thread
	Touch::SetPENCallback(cb, ptr);
}

bool Input::LoadCalibration() {
	if (File::Open("TOUCH.CAL", FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		LOG(Log_Input, LevelError, "Failed to open calibration file");
		return false;
	}
	if (File::ReadParameters(touchCal, 4) != File::ParameterResult::OK) {
		LOG(Log_Input, LevelError, "Calibration file incomplete");
		File::Close();
		return false;
	} else {
		File::Close();
		return true;
	}
}
