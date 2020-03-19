/*
 * menuentry.hpp
 *
 *  Created on: Jun 9, 2019
 *      Author: jan
 */

#ifndef MENUENTRY_HPP_
#define MENUENTRY_HPP_

#include <widget.hpp>
#include "display.h"

class MenuEntry : public Widget {
public:
	MenuEntry(){selectable = false;};
	virtual ~MenuEntry(){};
protected:
};



#endif /* MENUENTRY_HPP_ */
