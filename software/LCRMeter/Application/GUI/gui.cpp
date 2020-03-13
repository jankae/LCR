#include "gui.hpp"
#include "log.h"
#include "App.hpp"

QueueHandle_t GUIeventQueue = NULL;
Widget *topWidget;
bool isPopup;

TaskHandle_t GUIHandle;

static void guiThread(void) {
	LOG(Log_GUI, LevelInfo, "Thread start");
	GUIHandle = xTaskGetCurrentTaskHandle();

	GUIEvent_t event;

	display_SetBackground(COLOR_BLACK);
	display_Clear();

//	Button *test = new Button("Test", Font_Big, nullptr);
//
//	topWidget = test;

	while (1) {
		if (xQueueReceive(GUIeventQueue, &event, 300)) {
			if (topWidget) {
				switch (event.type) {
				case EVENT_TOUCH_PRESSED:
				case EVENT_TOUCH_RELEASED:
				case EVENT_TOUCH_HELD:
				case EVENT_TOUCH_DRAGGED:
					/* these are position based events */
					/* check if applicable for top widget
					 * (which, for smaller windows, might not be the case */
					if (topWidget->isInArea(event.pos)) {
						/* send event to top widget */
						Widget::input(topWidget, &event);
					}
					break;
//				case EVENT_BUTTON_CLICKED:
//					/* no break */
//				case EVENT_ENCODER_MOVED:
//					/* these events are always valid for the selected widget */
//					if (Widget::getSelected()) {
//						Widget::input(Widget::getSelected(), &event);
//					} else {
//						desktop_Input(&event);
//					}
//					break;
				case EVENT_WINDOW_CLOSE:
					topWidget->requestRedrawFull();
					break;
				case EVENT_APP_START:
					event.app->Start();
					break;
				case EVENT_APP_STARTED:
					if (event.app->topWidget) {
						event.app->d->addChild(event.app->topWidget,
								COORDS(40, 0));
						event.app->d->requestRedrawFull();
						event.app->d->FocusOnApp(event.app);
					}
					break;
				case EVENT_APP_STOP:
					event.app->Stop();
					break;
				case EVENT_APP_EXITED:
					if (event.app->topWidget) {
						delete event.app->topWidget;
						event.app->d->requestRedrawFull();
					}
					break;
				default:
					break;
				}
			}
		}
		if (topWidget) {
			Widget::draw(topWidget, COORDS(0, 0));
		}
	}
}

bool GUI::Init(Desktop& d) {
	/* initialize event queue */
	GUIeventQueue = xQueueCreate(10, sizeof(GUIEvent_t));

	if(!GUIeventQueue) {
		return false;
	}

	/* create GUI thread */
	if(xTaskCreate((TaskFunction_t )guiThread, "GUI", 300, NULL, 3, NULL)!=pdPASS) {
		return false;
	}

	topWidget = &d;
	return true;
}

bool GUI::SendEvent(GUIEvent_t* ev) {
	if(!GUIeventQueue || !ev) {
		/* some pointer error */
		return false;
	}
	BaseType_t yield = pdFALSE;
	xQueueSendFromISR(GUIeventQueue, ev, &yield);
	portYIELD_FROM_ISR(yield);
	return true;
}
