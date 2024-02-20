#pragma once

#include <cstdint>
#include <functional>
#include "file.hpp"

namespace Config {

using WriteFunc = std::function<bool (void)>;
using ReadFunc = std::function<bool (void)>;

void Init();
int AddParseFunctions(WriteFunc write, ReadFunc read);
bool RemoveParseFunctions(int index);

bool Store(const char *filename);
bool Load(const char *filename);

}
