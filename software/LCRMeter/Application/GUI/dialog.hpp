/*
 * dialog.h
 *
 *  Created on: Mar 16, 2017
 *      Author: jan
 */

#ifndef DIALOG_H_
#define DIALOG_H_

#include "gui.hpp"

namespace Dialog {

enum class MsgBox {
	ABORT_OK, OK
};

enum class Result {
	OK, ABORT, ERR
};

using Callback = void(*)(void *ptr, Result r);

Result MessageBox(const char *title, font_t font, const char *msg,
		MsgBox type, void (*cb)(Result), uint8_t block);

Result FileChooser(const char *title, char *result,
		const char *dir, const char *filetype);

/*
 * With block = true, the function blocks until a button is pressed and the
 * return value indicates which button has been pressed. Must not be called from GUI
 * thread context with block = true.
 * With block = false, the function returns immediately
 */
bool StringInput(const char *title, char *result, uint8_t maxLength,
		Callback cb, void *ptr);

Result StringInputBlock(const char *title, char *result, uint8_t maxLength);

Result UnitInput(const char *title, int32_t *result, uint8_t maxLength, const Unit::unit *unit[]);

}

#endif /* DIALOG_H_ */
