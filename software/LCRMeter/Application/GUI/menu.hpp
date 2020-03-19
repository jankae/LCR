/*
 * menu.hpp
 *
 *  Created on: Jun 9, 2019
 *      Author: jan
 */

#ifndef MENU_HPP_
#define MENU_HPP_

#include <widget.hpp>
#include "display.h"
#include "menuentry.hpp"

class Menu : public MenuEntry {
public:
	Menu(const char *name, coords_t size);
	~Menu();
	/*
	 * Adds a new entry to the menu.
	 * Optionally specify the position where the entry will be added.
	 * 	Position positive: 	Add entry at this position if menu has
	 * 						sufficient number of entries. Otherwise
	 * 						the entry will be added at the end
	 * 	Position negative: 	Position specified from the end of the
	 * 						menu. If the menu has less entries than
	 * 						abs(position), the new entry will be added
	 * 						as the first entry
	 */
	bool AddEntry(MenuEntry *e, int8_t position = INT8_MAX);
	bool RemoveEntry(MenuEntry *e);
private:
	void draw(coords_t offset) override;
	void input(GUIEvent_t *ev) override;
	void drawChildren(coords_t offset) override;

	Widget::Type getType() override { return Widget::Type::Menu; };

	void PageSwitched();
	void moveUp();
	void moveDown();

//	static constexpr int16_t EntrySizeX = 60;
	static constexpr int16_t EntrySizeY = 40;
	static constexpr color_t Foreground = COLOR_FG_DEFAULT;
	static constexpr color_t Selected = COLOR_SELECTED;
	static constexpr color_t Background = COLOR_BG_DEFAULT;

    uint8_t nentries, selectedEntry;
    uint8_t entriesPerPage;
    bool usePages :1;
    bool inSubMenu :1;
    char *name;
};


#endif /* MENU_HPP_ */
