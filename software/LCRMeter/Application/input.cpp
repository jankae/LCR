#include "input.h"

static void inputThread(void) {
	coords_t lastTouch;
	coords_t initialTouch;
	TickType_t touchStart = 0;
	uint8_t touchMoved = 0;
	lastTouch.x = -1;

	while (1) {
		vTaskDelay(pdMS_TO_TICKS(20));
		/* check touch for new input */
		coords_t touch;
		if (touch_GetCoordinates(&touch) > 0) {
			if (lastTouch.x == -1) {
				/* touch wasn't pressed before */
				initialTouch = touch;
				touchStart = xTaskGetTickCount();
				touchMoved = 0;
				/* touch press event */
				GUIEvent_t ev;
				ev.type = EVENT_TOUCH_PRESSED;
				ev.pos = touch;
				GUI::SendEvent(&ev);
				lastTouch = touch;
			} else {
				/* check if (large enough) movement detected */
				if (!touchMoved) {
					if (abs(initialTouch.x - touch.x) > 20
							|| abs(initialTouch.y - touch.y) > 20) {
						touchMoved = 1;
					} else if (xTaskGetTickCount() - touchStart > 1000) {
						/* touch held event */
						GUIEvent_t ev;
						ev.type = EVENT_TOUCH_HELD;
						ev.pos = initialTouch;
						GUI::SendEvent(&ev);
						/* mark as moved (prevents further touch held events) */
						touchMoved = 1;
					}
				}
				/* touch drag event */
				GUIEvent_t ev;
				touchMoved = 1;
				ev.type = EVENT_TOUCH_DRAGGED;
				ev.pos = initialTouch;
				ev.dragged = touch;
				GUI::SendEvent(&ev);
				lastTouch = touch;
			}
		} else {
			if (lastTouch.x != -1) {
				/* touch was pressed before */
				/* touch release event */
				GUIEvent_t ev;
				ev.type = EVENT_TOUCH_RELEASED;
				ev.pos = lastTouch;
				GUI::SendEvent(&ev);
				lastTouch.x = -1;
			}
		}
	}
}

bool input_Init(){
	/* create GUI thread */
	return xTaskCreate((TaskFunction_t )inputThread, "input", 100, NULL, 3, NULL) == pdPASS;
}
