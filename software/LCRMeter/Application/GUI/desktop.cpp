#include "desktop.hpp"
#include "dialog.hpp"
#include "Config.hpp"
#include "file.hpp"
#include "log.h"

Desktop::Desktop() {
	AppCnt = 0;
	apps.fill(nullptr);
	focussed = -1;

	size.x = DISPLAY_WIDTH;
	size.y = DISPLAY_HEIGHT;

	configIndex = Config::AddParseFunctions(
			[&]() { return this->WriteConfig(); },
			[&]() { return this->ReadConfig(); });
}

Desktop::~Desktop() {
	Config::RemoveParseFunctions(configIndex);
}

bool Desktop::AddApp(App& app) {
	if(AppCnt >= MaxApps) {
		return false;
	}
	apps[AppCnt] = &app;
	AppCnt++;
	return true;
}

bool Desktop::FocusOnApp(App* app) {
	for (uint8_t i = 0; i < AppCnt; i++) {
		if (apps[i] == app) {
			if (focussed != i) {
				/* bring app into focus */
				if (focussed != -1 && apps[focussed]->topWidget) {
					apps[focussed]->topWidget->setSelectable(false);
				}
				focussed = i;
				if (apps[i]->topWidget) {
					apps[focussed]->topWidget->setSelectable(true);
					apps[i]->topWidget->requestRedrawFull();
				}
				this->requestRedraw();
			}
			return true;
		}
	}
	return false;
}

void Desktop::draw(coords_t offset) {
	// always focus on running app
	if (focussed == -1 || apps[focussed]->state != App::State::Running) {
		// app was closed, switch focus to next app
		uint8_t i;
		for (i = 0; i < AppCnt; i++) {
			if (apps[i]->state == App::State::Running) {
				focussed = i;
				apps[i]->topWidget->setSelectable(true);
				break;
			}
		}
		if (i == AppCnt) {
			// no apps running anymore
			focussed = -1;
		}
	}
	/* clear desktop area */
	display_SetForeground(COLOR_BLACK);
	display_RectangleFull(offset.x, offset.y, offset.x + IconBarWidth - 1,
			offset.y + DISPLAY_HEIGHT - 1);
	uint8_t i;
	for (i = 0; i < AppCnt; i++) {
		if (apps[i]->state == App::State::Running
				|| apps[i]->state == App::State::Starting) {
			display_Image(offset.x + IconOffsetX,
					offset.y + i * IconSpacing + IconOffsetY,
					apps[i]->info.icon);
		} else {
			display_ImageGrayscale(offset.x + IconOffsetX,
					offset.y + i * IconSpacing + IconOffsetY,
					apps[i]->info.icon);
		}
		if (focussed == i) {
			/* this is the active app */
			display_SetForeground(COLOR_BG_DEFAULT);
		} else {
			display_SetForeground(COLOR_BLACK);
		}
		display_VerticalLine(offset.x + 0, offset.y + i * IconSpacing + 3,
				IconSpacing - 6);
		display_VerticalLine(offset.x + 1, offset.y + i * IconSpacing + 1,
				IconSpacing - 2);
		display_HorizontalLine(offset.x + 3, offset.y + i * IconSpacing,
				IconBarWidth - 3);
		display_HorizontalLine(offset.x + 2, offset.y + i * IconSpacing + 1,
				IconBarWidth - 2);
		display_HorizontalLine(offset.x + 3,
				offset.y + (i + 1) * IconSpacing - 1, IconBarWidth - 3);
		display_HorizontalLine(offset.x + 2,
				offset.y + (i + 1) * IconSpacing - 2, IconBarWidth - 2);
	}
	if (focussed == -1) {
		// clear whole screen area
		display_SetForeground(COLOR_BLACK);
		display_RectangleFull(offset.x + IconBarWidth, offset.y,
				offset.x + DISPLAY_WIDTH - 1, offset.y + DISPLAY_HEIGHT - 1);
		/* Add app names in app area */
		display_SetBackground(Background);
		display_SetForeground(Foreground);
		for (i = 0; i < AppCnt; i++) {
			display_SetFont(Font_Big);
			display_String(offset.x + IconBarWidth + 10,
					offset.y + i * IconSpacing
							+ (IconSpacing - Font_Big.height) / 2 - 4,
					apps[i]->info.name);

			display_SetFont(Font_Medium);
			display_String(offset.x + IconBarWidth + 10,
					offset.y + i * IconSpacing
							+ (IconSpacing - Font_Big.height) / 2 + 13,
					apps[i]->info.descr);
		}
	}
}
#include "log.h"
void Desktop::input(GUIEvent_t* ev) {
	switch (ev->type) {
	case EVENT_TOUCH_PRESSED:
		/* get icon number */
		if (ev->pos.x < IconBarWidth) {
			if (ev->pos.y < IconSpacing * AppCnt) {
				/* position is a valid icon */
				uint8_t app = ev->pos.y / IconSpacing;
				// switch to app
				switch (apps[app]->state) {
				case App::State::Stopped:
					/* start app */
				{
					GUIEvent_t ev;
					ev.type = EVENT_APP_START;
					ev.app = apps[app];
					GUI::SendEvent(&ev);
				}
					break;
				case App::State::Running:
				case App::State::Starting:
					FocusOnApp(apps[app]);
					break;
				default:
					/* do nothing */
					break;
				}
			}
			ev->type = EVENT_NONE;
		}
		break;
	case EVENT_TOUCH_HELD:
		/* get icon number */
		if (ev->pos.x < IconBarWidth) {
			if (ev->pos.y < IconSpacing * AppCnt) {
				/* position is a valid icon */
				uint8_t app = ev->pos.y / IconSpacing;
				switch (apps[app]->state) {
				case App::State::Running:
				case App::State::Starting:
					static App *AppToClose;
					AppToClose = apps[app];
					Dialog::MessageBox("Close?", Font_Big, "Close this app?",
							Dialog::MsgBox::ABORT_OK, [](Dialog::Result res) {
								if(res == Dialog::Result::OK) {
									GUIEvent_t ev;
									ev.type = EVENT_APP_STOP;
									ev.app = AppToClose;
									GUI::SendEvent(&ev);
								}
							}, false);
					break;
				default:
					/* do nothing */
					break;
				}
			}
			ev->type = EVENT_NONE;
		}
		break;
	default:
		break;
	}
}

void Desktop::drawChildren(coords_t offset) {
	if (focussed != -1 && apps[focussed]->topWidget) {
		Widget::draw(apps[focussed]->topWidget, offset);
	}
}

bool Desktop::WriteConfig() {
	File::Write("# Apps configuration\n");
	for (uint8_t i = 0; i < AppCnt; i++) {
		char name[50] = "App::";
		strncat(name, apps[i]->info.name, sizeof(name) - 15);
		strcat(name, "::Running");
		bool running = apps[i]->state == App::State::Running;
		File::Entry entry = { name, &running, File::PointerType::BOOL };
		File::WriteParameters(&entry, 1);
	}
	return true;
}

bool Desktop::ReadConfig() {
	for (uint8_t i = 0; i < AppCnt; i++) {
		char name[50] = "App::";
		strncat(name, apps[i]->info.name, sizeof(name) - 15);
		strcat(name, "::Running");
		bool running = false;
		File::Entry entry = { name, &running, File::PointerType::BOOL };
		File::ReadParameters(&entry, 1);
		/*
		 * Start/stop apps to comply to configuration. Needs to wait for
		 * the app to actually start/stop because the apps themselves might
		 * add/remove configuration read functions
		 */
		if (running && apps[i]->state == App::State::Stopped) {
			GUIEvent_t ev;
			ev.type = EVENT_APP_START;
			ev.app = apps[i];
			GUI::SendEvent(&ev);
			constexpr uint32_t maxStartDelay = 2500;
			uint32_t start = HAL_GetTick();
			while (apps[i]->state != App::State::Running) {
				vTaskDelay(10);
				if (HAL_GetTick() - start > maxStartDelay) {
					LOG(Log_Desktop, LevelWarn, "App didn't start in time");
				}
			}
		} else if(!running && apps[i]->state == App::State::Running) {
			GUIEvent_t ev;
			ev.type = EVENT_APP_STOP;
			ev.app = apps[i];
			GUI::SendEvent(&ev);
			constexpr uint32_t maxStopDelay = 2500;
			uint32_t start = HAL_GetTick();
			while (apps[i]->state != App::State::Stopped) {
				vTaskDelay(10);
				if (HAL_GetTick() - start > maxStopDelay) {
					LOG(Log_Desktop, LevelWarn, "App didn't close in time");
				}
			}
		}
	}
	return true;
}
