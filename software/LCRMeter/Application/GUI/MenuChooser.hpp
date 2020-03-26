/*
 * MenuChooser.hpp
 *
 *  Created on: Jun 9, 2019
 *      Author: jan
 */

#ifndef MENUCHOOSER_HPP_
#define MENUCHOOSER_HPP_

#include "menuentry.hpp"

class MenuChooser : public MenuEntry {
public:
	MenuChooser(const char *name, const char * const *items, uint8_t *value,
			Callback cb = nullptr, void *ptr = nullptr, bool popup = true);

private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;
	void ChooserCallback(bool updated, uint8_t newval);

	Widget::Type getType() override { return Widget::Type::MenuChooser; };

	static constexpr color_t Background = COLOR_BG_DEFAULT;
	static constexpr color_t Foreground = COLOR_FG_DEFAULT;
	static constexpr uint8_t MaxNameLength = 10;

	Callback cb;
	void *ptr;
    uint8_t *value;
    bool popup;
    char name[MaxNameLength + 1];
    const char * const *items;
};



#endif /* MENUCHOOSER_HPP_ */
