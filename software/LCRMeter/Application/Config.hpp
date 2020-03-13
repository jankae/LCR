#pragma once

#include <cstdint>
#include "file.hpp"

namespace Config {

using WriteFunc = bool (*)(void *ptr);
using ReadFunc = bool (*)(void *ptr);

void Init();
int AddParseFunctions(WriteFunc write, ReadFunc read, void *ptr);
bool RemoveParseFunctions(int index);

bool Store(const char *filename);
bool Load(const char *filename);

}
