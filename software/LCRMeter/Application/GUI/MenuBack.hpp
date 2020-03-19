/*
 * MenuBool.hpp
 *
 *  Created on: Jun 9, 2019
 *      Author: jan
 */

#ifndef MENUBACK_HPP_
#define MENUBACK_HPP_

#include "menuentry.hpp"

class MenuBack : public MenuEntry {
public:
	MenuBack() {
		selectable = false;
	}
	;

private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override { ev->type = EVENT_NONE; };

	Widget::Type getType() override { return Widget::Type::MenuBack; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Foreground = COLOR_FG_DEFAULT;
};



#endif /* MENUBOOL_HPP_ */
