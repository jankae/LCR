#pragma once

#include "display.h"
#include "FreeRTOS.h"
#include "task.h"
#include "widget.hpp"

class Desktop;

class App {
	friend class Desktop;
public:
	using TaskFunc = void (*)(void*);
	using Info = struct {
		const char *name;
		const char *descr;
		uint32_t StackSize;
		const Image_t *icon;
		TaskFunc task;
	};
	App(Info info, Desktop &d);
	void StartComplete(Widget *top);
	void Exit();
	bool Closed() { return state == State::Stopping; };
private:
	bool Start();
	bool Stop();
	friend void guiThread(void);
	enum class State : uint8_t {
		Stopped,
		Starting,
		Running,
		Stopping,
	};

	State state;
	Info info;
	TaskHandle_t handle;
	Widget *topWidget;
	Desktop *d;
};
