#pragma once

#include "menuentry.hpp"

class MenuAction : public MenuEntry {
public:
	MenuAction(const char *name, Callback cb, void *ptr);
	~MenuAction();

private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;

	Widget::Type getType() override { return Widget::Type::MenuAction; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Foreground = COLOR_FG_DEFAULT;

	char *name;
	Callback cb;
	void *ptr;
};
