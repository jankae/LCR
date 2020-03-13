#pragma once

#include <cstdint>
#include "fatfs.h"

extern SemaphoreHandle_t fileAccess;

namespace File {

enum class PointerType : uint8_t {
	INT8,
	INT16,
	INT32,
	FLOAT,
	STRING,
	BOOL
};

enum class ParameterResult : uint8_t {
	OK,
	Partial,
	Error,
};

using Entry = struct entry {
	const char *name;
	void *ptr;
	PointerType type;
};

FRESULT Init();
FRESULT Open(const char *filename, BYTE mode);
FRESULT Close(void);
bool ReadLine(char *dest, uint16_t maxLen);
int Write(const char *line);
void WriteParameters(const Entry *paramList, uint8_t length);
ParameterResult ReadParameters(const Entry *paramList, uint8_t length);

}
