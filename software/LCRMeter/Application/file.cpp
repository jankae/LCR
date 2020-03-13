#include "file.hpp"

#include "FreeRTOS.h"
#include "semphr.h"
#include <cstring>
#include <cstdlib>

SemaphoreHandle_t fileAccess;
static FIL file;
static bool fileOpened = false;
static FATFS fatfs;

FRESULT File::Init() {
	fileAccess = xSemaphoreCreateMutex();
	if (!fileAccess) {
		return FR_INT_ERR;
	}

	/* Check SD card */
	return f_mount(&fatfs, "0:/", 1);
}

FRESULT File::Open(const char* filename, BYTE mode) {
	if (xSemaphoreTake(fileAccess, 100)) {
		FRESULT res = f_open(&file, filename, mode);
		if (res == FR_OK) {
			fileOpened = true;
		} else {
			xSemaphoreGive(fileAccess);
		}
		return res;
	} else {
		return FR_DENIED;
	}
}

FRESULT File::Close(void) {
	FRESULT res;
	if (fileOpened) {
		res = f_close(&file);
		fileOpened = false;
		xSemaphoreGive(fileAccess);
	} else {
		return FR_NO_FILE;
	}
	return res;
}

bool File::ReadLine(char* dest, uint16_t maxLen) {
	return f_gets(dest, maxLen, &file) != nullptr;
}

int File::Write(const char* line) {
	return f_puts(line, &file);
}

void File::WriteParameters(const Entry *paramList,
		uint8_t length) {
	if (fileOpened) {
		/* opened file, now write parameters */
		uint8_t i;
		for (i = 0; i < length; i++) {
			f_puts(paramList[i].name, &file);
			f_puts(" = ", &file);
#define		MAX_PARAM_LENGTH		16
			char buf[MAX_PARAM_LENGTH + 2];
			switch (paramList[i].type) {
			case PointerType::INT8:
				snprintf(buf, sizeof(buf), "%d\n", *(int8_t*) paramList[i].ptr);
				break;
			case PointerType::INT16:
				snprintf(buf, sizeof(buf), "%d\n",
						*(int16_t*) paramList[i].ptr);
				break;
			case PointerType::INT32:
				snprintf(buf, sizeof(buf), "%ld\n",
						*(int32_t*) paramList[i].ptr);
				break;
			case PointerType::FLOAT:
				snprintf(buf, sizeof(buf), "%f\n", *(float*) paramList[i].ptr);
				break;
			case PointerType::STRING:
				snprintf(buf, sizeof(buf), "%s\n", (char*) paramList[i].ptr);
				break;
			case PointerType::BOOL:
				if(*(bool*) paramList[i].ptr) {
					strncpy(buf, "true\n", sizeof(buf));
				} else {
					strncpy(buf, "false\n", sizeof(buf));
				}
				break;
			default:
				strcpy(buf, "UNKNOWN TYPE\n");
			}
			f_puts(buf, &file);
		}
	}
}

File::ParameterResult File::ReadParameters(const Entry *paramList,
		uint8_t length) {
	if (fileOpened) {
		/* always start at beginning of file */
		f_lseek(&file, 0);
		/* opened file, now read parameters */
		char line[50];
		uint8_t valueSet[length];
		memset(valueSet, 0, sizeof(valueSet));
		while (f_gets(line, sizeof(line), &file)) {
			if (line[0] == '#') {
				/* skip comment lines */
				continue;
			}
			/* find matching parameter */
			uint8_t i;
			for (i = 0; i < length; i++) {
				if (!strncmp(line, paramList[i].name,
						strlen(paramList[i].name))) {
					/* found a match */
					char *start = strchr(line, '=');
					if (start) {
						/* Skip leading spaces */
						while (*++start == ' ')
							;
						// terminate at line end
						char *newline = strchr(start, '\n');
						if (newline) {
							*newline = 0;
						}
						char *cr = strchr(start, '\r');
						if (cr) {
							*cr = 0;
						}
						/* store value in correct format */
						switch (paramList[i].type) {
						case PointerType::INT8:
							*(int8_t*) paramList[i].ptr = (int8_t) strtol(start,
									NULL, 0);
							break;
						case PointerType::INT16:
							*(int16_t*) paramList[i].ptr = (int16_t) strtol(
									start, NULL, 0);
							break;
						case PointerType::INT32:
							*(int32_t*) paramList[i].ptr = (int32_t) strtol(
									start, NULL, 0);
							break;
						case PointerType::FLOAT:
							*(float*) paramList[i].ptr = strtof(start,
							NULL);
							break;
						case PointerType::STRING:
							strcpy((char*) paramList[i].ptr, start);
							break;
						case PointerType::BOOL:
							if (!strncmp(start, "true", 4)) {
								*(bool*) paramList[i].ptr = true;
							} else if (!strncmp(start, "false", 5)) {
								*(bool*) paramList[i].ptr = false;
							} else {
								return ParameterResult::Error;
							}
						}
						/* mark parameter as set */
						valueSet[i] = 1;
					}

				}
			}

			/* Check if all parameters have been set */
			for (i = 0; i < length; i++) {
				if (!valueSet[i]) {
					break;
				}
			}
			if(i==length) {
				/* all parameters have been set */
				return ParameterResult::OK;
			}
		}
	}
	/* file ended before all parameters have been set */
	return ParameterResult::Partial;
}
