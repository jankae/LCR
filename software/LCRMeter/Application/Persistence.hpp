#pragma once

#include <stdint.h>

namespace Persistence {

void Init();
bool Add(void *ptr, uint16_t size);
bool Save();
bool Load();

}
