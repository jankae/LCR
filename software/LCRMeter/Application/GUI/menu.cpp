#include "menu.hpp"
#include "log.h"
#include "Unit.hpp"

Menu::Menu(const char *name, coords_t size) {
	this->size = size;
	selectable = true;
	nentries = 0;
	selectedEntry = 0;
	usePages = false;
	entriesPerPage = size.y / EntrySizeY;
	redrawChild = true;
	inSubMenu = false;
	this->name = new char[strlen(name) + 1];
	strcpy(this->name, name);
}

Menu::~Menu() {
	delete name;
}

bool Menu::AddEntry(MenuEntry* e, int8_t position) {
	if (firstChild && position != 0 && position + nentries > 0) {
		uint8_t cnt = 1;
		/* find end of entry list */
		Widget *entry = firstChild;
		do {
			if (entry == e) {
				/* this widget has already been added, this must never happen */
				LOG(Log_GUI, LevelCrit, "Duplicate entry in menu");
				return false;
			}
			if ((position > 0 && cnt >= position)
					|| (position < 0 && cnt >= nentries + position)) {
				// reached requested position, insert here
				break;
			}
			if (entry->next) {
				entry = entry->next;
				cnt++;
			} else {
				break;
			}
		} while (1);
		/* add widget to the end */
		e->next = entry->next;
		entry->next = e;
	} else {
		/* this is the first child */
		e->next = firstChild;
		firstChild = e;
	}
	e->parent = this;
	if (e->getType() == Widget::Type::Menu) {
		// adding a submenu
		e->size = this->size;
		e->setPosition(COORDS(0, 0));
	} else {
		e->size.x = size.x - 2;
		e->size.y = EntrySizeY - 4;
		e->setPosition(COORDS(2, 2));
	}
	nentries++;
	entriesPerPage = size.y / EntrySizeY;
	if (nentries > entriesPerPage) {
		usePages = true;
		entriesPerPage--;
	}
	return true;
}

bool Menu::RemoveEntry(MenuEntry *e) {
	if (!firstChild) {
		// no entries available, can not remove any
		return false;
	}
	if(e == firstChild) {
		// Removing the first entry
		firstChild = e->next;
		e->parent = nullptr;
		e->next = nullptr;
	} else {
		// find entry before e
		MenuEntry *before = firstChild;
		while(before->next != e) {
			if (!before->next) {
				// reached end of list before finding e
				return false;
			} else {
				before = before->next;
			}
		}
		before->next = e->next;
		e->parent = nullptr;
		e->next = nullptr;
	}
	// If we get here, the entry has already been removed.
	// Adjust number of entries and paging
	nentries--;
	entriesPerPage = size.y / EntrySizeY;
	if (nentries <= entriesPerPage) {
		usePages = false;
	} else {
		// paging is still used, one less entry per page
		entriesPerPage--;
	}
	return true;
}

char* Menu::GetSelectedSubmenuName() {
	if (inSubMenu) {
		return ((Menu*) firstChild->GetNth(selectedEntry))->name;
	} else {
		return nullptr;
	}
}

void Menu::draw(coords_t offset) {
	if (!nentries) {
		LOG(Log_GUI, LevelCrit, "Menu needs at least one entry");
	}
	/* calculate corners */
	coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x;
	lowerRight.y += size.y;
	// trigger possible child redraw preventively here to allow menu entries to draw over menu screen
	if (redrawChild) {
		this->drawChildren(offset);
		redrawChild = false;
	}
	if (inSubMenu) {
		return;
	}
	// get offset of first entry on page
	uint8_t pageOffset = (uint8_t) (selectedEntry / entriesPerPage)
			* entriesPerPage;
//	MenuEntry *entry = static_cast<MenuEntry*>(firstChild->GetNth(pageOffset));
	for (uint8_t i = 0; i < entriesPerPage; i++) {
		if (i + pageOffset >= nentries) {
			break;
		}
		if (i + pageOffset == selectedEntry && selected) {
			display_SetForeground(Selected);
		} else {
			display_SetForeground(Foreground);
		}
		display_HorizontalLine(upperLeft.x + 1, upperLeft.y + i * EntrySizeY,
				size.x - 1);
		display_HorizontalLine(upperLeft.x + 1,
				upperLeft.y + i * EntrySizeY + 1, size.x - 1);
		display_HorizontalLine(upperLeft.x + 1,
				upperLeft.y + (i + 1) * EntrySizeY - 1, size.x - 1);
		display_HorizontalLine(upperLeft.x + 1,
				upperLeft.y + (i + 1) * EntrySizeY - 2, size.x - 1);
		display_VerticalLine(upperLeft.x,
				upperLeft.y + i * EntrySizeY + 1, EntrySizeY - 2);
		display_VerticalLine(upperLeft.x + 1,
				upperLeft.y + i * EntrySizeY + 1, EntrySizeY - 2);
//		entry->draw(
//				COORDS(offset.x + entry->position.x + 0,
//						offset.y + entry->position.y + i * EntrySizeY));
//		entry = static_cast<MenuEntry*>(entry->next);
	}
	if (usePages) {
		// draw page switcher
		display_SetFont(Font_Big);
		display_SetForeground(Foreground);
		display_SetBackground(Background);
		display_HorizontalLine(upperLeft.x + 1, upperLeft.y + entriesPerPage * EntrySizeY,
				size.x - 1);
		display_HorizontalLine(upperLeft.x + 1,
				upperLeft.y + entriesPerPage * EntrySizeY + 1, size.x - 1);
		display_HorizontalLine(upperLeft.x + 1,
				upperLeft.y + (entriesPerPage + 1) * EntrySizeY - 1,
				size.x - 1);
		display_HorizontalLine(upperLeft.x + 1,
				upperLeft.y + (entriesPerPage + 1) * EntrySizeY - 2,
				size.x - 1);
		display_VerticalLine(upperLeft.x,
				upperLeft.y + entriesPerPage * EntrySizeY + 1, EntrySizeY - 2);
		display_VerticalLine(upperLeft.x + 1,
				upperLeft.y + entriesPerPage * EntrySizeY + 1, EntrySizeY - 2);
		char page[4];
		page[0] = selectedEntry / entriesPerPage + 1 + '0';
		page[1] = '/';
		page[2] = (nentries - 1) / entriesPerPage + 1 + '0';
		page[3] = 0;
		uint16_t shiftX = (size.x - strlen(page) * Font_Big.width) / 2;
		uint16_t shiftY = (EntrySizeY - Font_Big.height) / 2;
		display_String(lowerRight.x - size.x + shiftX,
				upperLeft.y + entriesPerPage * EntrySizeY + shiftY, page);
	}
}

void Menu::drawChildren(coords_t offset) {
	if (inSubMenu) {
		// a submenu is active, draw it instead
		auto submenu = firstChild->GetNth(selectedEntry);
		Widget::draw(submenu, offset);
		return;
	}
	// get offset of first entry on page
	uint8_t pageOffset = (uint8_t) (selectedEntry / entriesPerPage)
			* entriesPerPage;
	MenuEntry *entry = static_cast<MenuEntry*>(firstChild->GetNth(pageOffset));
	for (uint8_t i = 0; i < entriesPerPage; i++) {
		if (i + pageOffset >= nentries) {
			break;
		}
		if (entry->getType() == Widget::Type::Menu) {
			// draw the submenu name
			auto submenu = static_cast<Menu*>(entry);
			coords_t upperLeft = COORDS(offset.x + size.x - size.x + 2,
					offset.y + i * EntrySizeY + 2);
			coords_t lowerRight = COORDS(offset.x + size.x,
					offset.y + (i + 1) * EntrySizeY - 2);
			display_SetForeground(Foreground);
			display_AutoCenterString(submenu->name, upperLeft, lowerRight);
		} else {
			Widget::draw(entry, COORDS(offset.x, offset.y + i * EntrySizeY));
		}
		entry = static_cast<MenuEntry*>(entry->next);
	}
}

void Menu::input(GUIEvent_t* ev) {
	if (inSubMenu) {
		// pass on event to submenu
		firstChild->GetNth(selectedEntry)->input(ev);
		return;
	}
	switch (ev->type) {
	case EVENT_ENCODER_MOVED: {
		if(ev->movement > 0) {
			moveDown();
		} else {
			moveUp();
		}
		ev->type = EVENT_NONE;
	}
		break;
	case EVENT_TOUCH_PRESSED: {
		// select new item
		uint8_t itemOnPage = ev->pos.y / EntrySizeY;
		if (usePages && itemOnPage >= entriesPerPage) {
			// the "next-page" area was touched, select first entry on next page
			uint8_t old_page = selectedEntry / entriesPerPage;
			selectedEntry = (old_page + 1) * entriesPerPage;
			if (selectedEntry >= nentries) {
				selectedEntry = 0;
			}
			PageSwitched();
			ev->type = EVENT_NONE;
		} else {
			// get offset of first entry on page
			uint8_t old_selected = selectedEntry;
			uint8_t pageOffset = (uint8_t) (selectedEntry / entriesPerPage)
					* entriesPerPage;
			uint8_t new_selected = ev->pos.y / EntrySizeY + pageOffset;
			if (new_selected >= nentries) {
				// touched at a spot where no entry is available
				ev->type = EVENT_NONE;
				break;
			}
			if (old_selected != new_selected) {
				selectedEntry = new_selected;
				this->requestRedraw();
			}
			auto sel = firstChild->GetNth(selectedEntry);
			if (sel->selectable && !sel->selected
					&& sel->getType() != Widget::Type::Menu) {
				sel->select();
				ev->type = EVENT_NONE;
			} else {
				select();
			}
			// do not reset ev->type as the touch event will be passed on to the selected entry
		}
	}
		break;
//	case EVENT_BUTTON_CLICKED: {
//		if (ev->button & BUTTON_ESC) {
//			if (parent) {
//				parent->select(false);
//				if (parent->getType() == Widget::Type::Menu) {
//					// leaving this submenu
//					auto parent_menu = static_cast<Menu*>(parent);
//					parent_menu->inSubMenu = false;
//					parent_menu->PageSwitched();
//				}
//			}
//			ev->type = EVENT_NONE;
//		} else if (ev->button & BUTTON_UP) {
//			moveUp();
//			ev->type = EVENT_NONE;
//		} else if (ev->button & BUTTON_DOWN) {
//			moveDown();
//			ev->type = EVENT_NONE;
//		}
//	}
	}
	if (ev->type != EVENT_NONE) {
		// pass on to selected entry
		if (ev->type == EVENT_TOUCH_DRAGGED || ev->type == EVENT_TOUCH_PRESSED
				|| ev->type == EVENT_TOUCH_HELD
				|| ev->type == EVENT_TOUCH_RELEASED) {
			// position based events, update event position
			ev->pos.x -= 2;
			ev->pos.y -= 2 + (selectedEntry % entriesPerPage) * EntrySizeY;
			;
		}
		auto entry = firstChild->GetNth(selectedEntry);
		if (entry->getType() == Widget::Type::Menu) {
			// the event is for a submenu
			if (ev->type == EVENT_TOUCH_PRESSED
					/*|| (ev->type == EVENT_BUTTON_CLICKED
							&& (ev->button & BUTTON_ENCODER))*/) {
				// entering submenu
				inSubMenu = true;
				entry->select();
				static_cast<Menu*>(entry)->PageSwitched();
				ev->type = EVENT_NONE;
			}
		} else if (entry->getType() == Widget::Type::MenuBack) {
			if (ev->type == EVENT_TOUCH_PRESSED
					/*| (ev->type == EVENT_BUTTON_CLICKED
							&& (ev->button & BUTTON_ENCODER))*/) {
				if (parent) {
					parent->select(false);
					if (parent->getType() == Widget::Type::Menu) {
						// leaving this submenu
						auto parent_menu = static_cast<Menu*>(parent);
						parent_menu->inSubMenu = false;
						parent_menu->PageSwitched();
					}
				}
				ev->type = EVENT_NONE;
			}
		} else {
			Widget::input(entry, ev);
		}
	}
}

void Menu::PageSwitched() {
	this->requestRedrawFull();
	uint8_t pageOffset = (uint8_t) (selectedEntry / entriesPerPage)
			* entriesPerPage;
	MenuEntry *entry = static_cast<MenuEntry*>(firstChild->GetNth(pageOffset));
	for (uint8_t i = 0; i < entriesPerPage; i++) {
		if (i + pageOffset >= nentries) {
			break;
		}
		entry->requestRedraw();
		entry = static_cast<MenuEntry*>(entry->next);
	}

}

void Menu::moveUp() {
	uint8_t old_page = selectedEntry / entriesPerPage;
	if (selectedEntry > 0) {
		selectedEntry--;
	} else {
		selectedEntry = nentries - 1;
	}
	uint8_t new_page = selectedEntry / entriesPerPage;
	if (new_page == old_page) {
		this->requestRedraw();
	} else {
		PageSwitched();
	}
}

void Menu::moveDown() {
	uint8_t old_page = selectedEntry / entriesPerPage;
	if (selectedEntry < nentries - 1) {
		selectedEntry++;
	} else {
		selectedEntry = 0;
	}
	uint8_t new_page = selectedEntry / entriesPerPage;
	if (new_page == old_page) {
		this->requestRedraw();
	} else {
		PageSwitched();
	}
}


