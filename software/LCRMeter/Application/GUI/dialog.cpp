#include "dialog.hpp"

#include "log.h"
#include "file.hpp"

extern TaskHandle_t GUIHandle;

namespace Dialog {

#define INPUT_DIALOG_LENGTH		10

struct {
	Window *window;
	union {
		struct {
			SemaphoreHandle_t dialogDone;
			Result res;
			void (*cb)(Result);
		} msgbox;
		struct {
			SemaphoreHandle_t dialogDone;
			uint8_t OKclicked;
		} fileChooser;
		struct {
			Callback cb;
			void *ptr;
			Window *w;
			char *string;
			char *destString;
			Label *lString;
			uint8_t pos;
			uint8_t maxLength;
		} StringInput;
		struct {
			SemaphoreHandle_t dialogDone;
		} UnitInput;
	};
} dialog;

static void MessageBoxButton(void*, Widget *w) {
	dialog.msgbox.res = Result::ERR;
	Button *b = (Button*) w;
	/* find which button has been pressed */
	if(!strcmp(b->getName(), "OK")) {
		dialog.msgbox.res = Result::OK;
	} else if(!strcmp(b->getName(), "ABORT")) {
		dialog.msgbox.res = Result::ABORT;
	}

	delete dialog.window;

	if (dialog.msgbox.cb)
		dialog.msgbox.cb(dialog.msgbox.res);

	if (dialog.msgbox.dialogDone) {
		xSemaphoreGive(dialog.msgbox.dialogDone);
	}
}

Result MessageBox(const char *title, font_t font, const char *msg,
		MsgBox type, void (*cb)(Result), uint8_t block){
	/* check pointers */
	if (!title || !msg) {
		return Result::ERR;
	}

	memset(&dialog, 0, sizeof(dialog));

	if(block && xTaskGetCurrentTaskHandle() == GUIHandle) {
		/* This dialog must never be called by the GUI thread (Deadlock) */
		LOG(Log_GUI, LevelCrit, "Dialog started from GUI thread.");
	}

	if (block) {
		dialog.msgbox.dialogDone = xSemaphoreCreateBinary();
	}

	/* create dialog window and elements */
	Textfield *text = new Textfield(msg, font);
	/* calculate window size */
	coords_t windowSize = text->getSize();
	if (windowSize.x < 132) {
		windowSize.x = 136;
	} else {
		windowSize.x += 4;
	}
	windowSize.y += 50;
	Window *w = new Window(title, Font_Big, windowSize);
	Container *c = new Container(w->getAvailableArea());
	c->attach(text, COORDS(1, 2));
	switch (type) {
	case MsgBox::OK: {
		Button *bOK = new Button("OK", Font_Big, MessageBoxButton, nullptr, COORDS(65, 0));
		c->attach(bOK, COORDS((c->getSize().x - bOK->getSize().x) / 2,
						c->getSize().y - bOK->getSize().y - 1));
//		bOK->select();
	}
	break;
	case MsgBox::ABORT_OK: {
		Button *bOK = new Button("OK", Font_Big, MessageBoxButton, nullptr, COORDS(65, 0));
		Button *bAbort = new Button("ABORT", Font_Big, MessageBoxButton, nullptr, COORDS(65, 0));
		c->attach(bAbort,
				COORDS(c->getSize().x / 2 - bAbort->getSize().x - 1,
						c->getSize().y - bAbort->getSize().y - 1));
		c->attach(bOK,
				COORDS(c->getSize().x / 2 + 1,
						c->getSize().y - bOK->getSize().y - 1));
//		bAbort->select();
	}
		break;
	}

	dialog.window = w;
	dialog.msgbox.cb = cb;

	w->setMainWidget(c);

	if(block) {
		/* wait for dialog to finish and return result */
		xSemaphoreTake(dialog.msgbox.dialogDone, portMAX_DELAY);
		vPortFree(dialog.msgbox.dialogDone);
		return dialog.msgbox.res;
	} else {
		/* non blocking mode, return immediately */
		return Result::OK;
	}
}

Result FileChooser(const char *title, char *result,
		const char *dir, const char *filetype) {
	if(xTaskGetCurrentTaskHandle() == GUIHandle) {
		/* This dialog must never be called by the GUI thread (Deadlock) */
		LOG(Log_GUI, LevelCrit, "Dialog started from GUI thread.");
	}

	/* check pointers */
	if (!title || !dir) {
		return Result::ERR;
	}

	memset(&dialog, 0, sizeof(dialog));

	dialog.fileChooser.dialogDone = xSemaphoreCreateBinary();
	if(!dialog.fileChooser.dialogDone) {
		/* failed to create semaphore */
		return Result::ERR;
	}

	/* Find applicable files */
	if(!xSemaphoreTake(fileAccess, 1000)) {
		/* failed to allocate fileAccess */
		return Result::ERR;
	}

#define MAX_NUMBER_OF_FILES		50
	char *filenames[MAX_NUMBER_OF_FILES + 1];
	uint8_t foundFiles = 0;
	FRESULT fr; /* Return value */
	DIR *dj = (DIR*) pvPortMalloc(sizeof(DIR)); /* Directory search object */
	FILINFO fno; /* File information */

#if _USE_LFN
	static char lfn[_MAX_LFN + 1]; /* Buffer to store the LFN */
	fno.lfname = lfn;
	fno.lfsize = sizeof lfn;
#endif

	fr = f_opendir(dj, dir);

	if (fr == FR_OK) {
		while (f_readdir(dj, &fno) == FR_OK) {
			char *fn;
#if _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
#else
			fn = fno.fname;
#endif
			if(!fn[0] || foundFiles >= MAX_NUMBER_OF_FILES)
				/* no more files */
				break;
			if (filetype) {
				/* check file for filetype */
				char *typestart = strchr(fn, '.');
				if (!typestart) {
					/* file has not type */
					continue;
				}
				typestart++;
				if (strcmp(typestart, filetype)) {
					/* type doesn't match */
					continue;
				}
			}
			/* allocate memory for filename */
			filenames[foundFiles] = (char*) pvPortMalloc(strlen(fn) + 1);
			if (!filenames[foundFiles]) {
				/* malloc failed */
				/* free already allocated names and return with error */
				while (foundFiles > 0) {
					foundFiles--;
					vPortFree(filenames[foundFiles]);
				}
				vPortFree(dj);
				return Result::ERR;
			}
			/* copy filename */
			strcpy(filenames[foundFiles], fn);
			/* switch to next filename */
			foundFiles++;
		}
		f_closedir(dj);
	}
	vPortFree(dj);
	xSemaphoreGive(fileAccess);
	/* Got all matching filenames */
	/* mark end of filename strings */
	filenames[foundFiles] = 0;

	/* Create window */
	Window *w = new Window(title, Font_Big, COORDS(280, 200));
	Container *c = new Container(w->getAvailableArea());

	Button *bAbort = new Button("ABORT", Font_Big, [](void*, Widget *w) {
		dialog.fileChooser.OKclicked = 0;
		xSemaphoreGive(dialog.fileChooser.dialogDone);
	}, nullptr, COORDS(80, 0));

	uint8_t selectedFile = 0;
	if (foundFiles) {
		ItemChooser *i = new ItemChooser((const char**) filenames,
				&selectedFile, Font_Big,
				(c->getSize().y - 30) / Font_Big.height, c->getSize().x);
		c->attach(i, COORDS(0, 0));
		Button *bOK = new Button("OK", Font_Big, [](void*, Widget *w) {
			dialog.fileChooser.OKclicked = 1;
			xSemaphoreGive(dialog.fileChooser.dialogDone);
		}, nullptr, COORDS(80, 0));
		c->attach(bOK,
				COORDS(c->getSize().x - bOK->getSize().x - 5,
						c->getSize().y - bOK->getSize().y - 5));
//		i->select();
	} else {
		/* got no files */
		Label *lNoFiles = new Label("No files available", Font_Big);
		c->attach(lNoFiles,
				COORDS((c->getSize().x - lNoFiles->getSize().x) / 2, 40));
//		bAbort->select();
	}


	c->attach(bAbort,
			COORDS(5, c->getSize().y - bAbort->getSize().y - 5));

	w->setMainWidget(c);

	/* Wait for button to be clicked */
	xSemaphoreTake(dialog.fileChooser.dialogDone, portMAX_DELAY);
	vPortFree(dialog.fileChooser.dialogDone);

	if(dialog.fileChooser.OKclicked) {
		strcpy(result, filenames[selectedFile]);
	}

	/* free all allocated filenames */
	while (foundFiles > 0) {
		foundFiles--;
		vPortFree(filenames[foundFiles]);
	}

	/* delete window */
	delete w;

	if(dialog.fileChooser.OKclicked) {
		return Result::OK;
	} else {
		return Result::ERR;
	}
}

static void stringInputChar(char c) {
	if(c == 0x08) {
		/* backspace, delete last char */
		if(dialog.StringInput.pos>0) {
			dialog.StringInput.pos--;
			dialog.StringInput.string[dialog.StringInput.pos] = 0;
			dialog.StringInput.lString->setText(dialog.StringInput.string);
		}
	} else {
		/* append character if space is available */
		if(dialog.StringInput.pos<dialog.StringInput.maxLength - 1) {
			dialog.StringInput.string[dialog.StringInput.pos] = c;
			dialog.StringInput.pos++;
			dialog.StringInput.lString->setText(dialog.StringInput.string);
		}
	}
}

bool StringInput(const char *title, char *result, uint8_t maxLength,
		Callback cb, void *ptr) {
	/* check pointers */
	if (!title || !result) {
		return false;
	}

	memset(&dialog, 0, sizeof(dialog));

	dialog.StringInput.cb = cb;
	dialog.StringInput.ptr = ptr;
	dialog.StringInput.string = new char[maxLength + 1];
	strncpy(dialog.StringInput.string, result, maxLength);
	dialog.StringInput.destString = result;
	dialog.StringInput.maxLength = maxLength;
	dialog.StringInput.pos = strlen(dialog.StringInput.string);

	memset(result, 0, maxLength);

	/* Create window */
	dialog.StringInput.w = new Window(title, Font_Big, COORDS(313, 233));
	Container *c = new Container(dialog.StringInput.w->getAvailableArea());

	Keyboard *k = new Keyboard(stringInputChar);

	dialog.StringInput.lString = new Label(
			c->getSize().x / Font_Big.width, Font_Big, Label::Orientation::CENTER);

	dialog.StringInput.lString->setText(dialog.StringInput.string);

	/* Create buttons */
	Button *bOK = new Button("OK", Font_Big, [](void*, Widget *w) {
		strcpy(dialog.StringInput.destString, dialog.StringInput.string);
		if(dialog.StringInput.cb) {
			dialog.StringInput.cb(dialog.StringInput.ptr, Result::OK);
		}
		delete dialog.StringInput.w;
	}, nullptr, COORDS(80, 0));
	Button *bAbort = new Button("ABORT", Font_Big, [](void*, Widget *w) {
		if(dialog.StringInput.cb) {
			dialog.StringInput.cb(dialog.StringInput.ptr, Result::ABORT);
		}
		delete dialog.StringInput.w;
	}, nullptr, COORDS(80, 0));

	c->attach(dialog.StringInput.lString, COORDS(0, 8));
	c->attach(k, COORDS(0, 30));
	c->attach(bOK,
			COORDS(c->getSize().x - bOK->getSize().x - 5,
					c->getSize().y - bOK->getSize().y - 5));
	c->attach(bAbort,
			COORDS(5, c->getSize().y - bAbort->getSize().y - 5));

//	k->select();
	dialog.StringInput.w->setMainWidget(c);

	return true;
}


Result StringInputBlock(const char* title, char* result, uint8_t maxLength) {
	if(xTaskGetCurrentTaskHandle() == GUIHandle) {
		/* This dialog must never be called by the GUI thread (Deadlock) */
		LOG(Log_GUI, LevelCrit, "Dialog started from GUI thread.");
	}

	using DataStruct = struct {
		SemaphoreHandle_t semphr;
		Result res;
	};
	DataStruct data;

	data.semphr = xSemaphoreCreateBinary();
	if(!StringInput(title, result, maxLength, [](void *ptr, Result r) {
			DataStruct *d = (DataStruct*) ptr;
			d->res = r;
			xSemaphoreGive(d->semphr);
		}, &data)) {
		return Result::ERR;
	}

	xSemaphoreTake(data.semphr, portMAX_DELAY);
	vSemaphoreDelete(data.semphr);

	return data.res;
}


Result UnitInput(const char *title, int32_t *result, uint8_t maxLength, const Unit::unit *unit[]) {
	if(xTaskGetCurrentTaskHandle() == GUIHandle) {
		/* This dialog must never be called by the GUI thread (Deadlock) */
		LOG(Log_GUI, LevelCrit, "Dialog started from GUI thread.");
	}

	/* check pointers */
	if (!title || !result) {
		return Result::ERR;
	}

	memset(&dialog, 0, sizeof(dialog));

	dialog.UnitInput.dialogDone = xSemaphoreCreateBinary();
	if(!dialog.UnitInput.dialogDone) {
		/* failed to create semaphore */
		return Result::ERR;
	}

	*result = 0;

	/* Create window */
	Window *w = new Window(title, Font_Big, COORDS(313, 233));
	Container *c = new Container(w->getAvailableArea());

	Entry *e = new Entry(result, nullptr, nullptr, Font_Big, maxLength, unit);

	/* Create buttons */
	Button *bOK = new Button("OK", Font_Big, [](void*, Widget *w) {
		xSemaphoreGive(dialog.UnitInput.dialogDone);
	}, nullptr, COORDS(80, 0));


	c->attach(e, COORDS((c->getSize().x - e->getSize().x) / 2, 5));
	c->attach(bOK,
			COORDS((c->getSize().x - bOK->getSize().x) / 2,
					c->getSize().y - bOK->getSize().y - 5));

//	e->select();
	w->setMainWidget(c);

	/* Wait for button to be clicked */
	xSemaphoreTake(dialog.UnitInput.dialogDone, portMAX_DELAY);
	vPortFree(dialog.UnitInput.dialogDone);

	/* delete window */
	delete w;

	return Result::OK;
}

}
