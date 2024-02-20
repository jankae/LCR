#ifndef GUI_GUI_H_
#define GUI_GUI_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "button.hpp"
#include "container.hpp"
#include "window.hpp"
#include "label.hpp"
#include "dialog.hpp"
#include "entry.hpp"
#include "textfield.hpp"
#include "checkbox.hpp"
#include "sevensegment.hpp"
#include "itemChooser.hpp"
#include "keyboard.hpp"
#include "progressbar.hpp"
#include "graph.hpp"
#include "custom.hpp"
#include "Radiobutton.hpp"
#include "slider.hpp"
#include "menu.hpp"
#include "MenuBack.hpp"
#include "MenuBool.hpp"
#include "MenuChooser.hpp"
#include "MenuValue.hpp"
#include "MenuAction.hpp"
#include "EventCatcher.hpp"

#include "desktop.hpp"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 (0.707106781186547524401)
#endif

namespace GUI {

bool Init(Widget& top);

bool SendEvent(GUIEvent_t *ev);

}

#endif
