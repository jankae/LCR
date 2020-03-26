#include "Persistence.hpp"

#include <string.h>
#include <util.h>
#include "log.h"
#include "stm.h"

#define Log_Persistence	(LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

static constexpr uint32_t maxSize = 2048;
static constexpr uint32_t usableSize = maxSize - 4;
static constexpr uint16_t pagesize = 2048;
static constexpr uint32_t FLASHend = 0x08080000;

static_assert(maxSize%pagesize == 0);

using Entry = struct {
	void *ptr;
	uint32_t offset;
	uint16_t size;
};

static constexpr uint8_t maxEntries = 10;
static Entry entries[maxEntries];

void Persistence::Init() {
	memset(entries, 0, sizeof(entries));
}

bool Persistence::Add(void* ptr, uint16_t size) {
	uint32_t freeOffset = 0;
	for(uint8_t i=0;i<maxEntries;i++) {
		if(entries[i].ptr) {
			freeOffset = entries[i].offset + entries[i].size;
		} else {
			if (freeOffset + size <= usableSize) {
				// found an empty spot, add data
				entries[i].ptr = ptr;
				// offset should be at 16bit boundary
				freeOffset = (freeOffset + 1) & 0xFFFFFFFE;
				entries[i].offset = freeOffset;
				entries[i].size = size;
				return true;
			} else {
				LOG(Log_Persistence, LevelCrit, "Unable to add data to persistence, exeeded size");
				return false;
			}
		}
	}
	LOG(Log_Persistence, LevelCrit, "Unable to add data to persistence, exeeded number of entries");
	return false;
}

bool Persistence::Save() {
	HAL_FLASH_Unlock();
	uint8_t pagesToErase = maxSize / pagesize;
	uint32_t start = FLASHend - maxSize;
	FLASH_EraseInitTypeDef erase;
	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	erase.NbPages = pagesToErase;
	erase.PageAddress = start;
	uint32_t perror;
	if (HAL_FLASHEx_Erase(&erase, &perror) != HAL_OK) {
		HAL_FLASH_Lock();
		return false;
	}

	for (uint8_t i = 0; i < maxEntries; i++) {
		if (entries[i].ptr) {
			for (uint16_t j = 0; j < entries[i].size; j += 2) {
				if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
						start + entries[i].offset + j,
						*(uint16_t*) ((uint8_t*) entries[i].ptr + j))
						!= HAL_OK) {
					HAL_FLASH_Lock();
					return false;
				}
			}
		}
	}

	uint32_t crc = util_crc32(0, (uint8_t*) start, usableSize);
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASHend - 4, crc)
			!= HAL_OK) {
		HAL_FLASH_Lock();
		return false;
	}
	HAL_FLASH_Lock();
	return true;
}

bool Persistence::Load() {
	uint8_t *start = (uint8_t*) (FLASHend - maxSize);
	uint32_t crc = util_crc32(0, start, usableSize);
	// compare with last FLASH word (should match CRC)
	uint32_t compare = *(uint32_t*) (FLASHend - 4);

	if (crc != compare) {
		return false;
	} else {
		for (uint8_t i = 0; i < maxEntries; i++) {
			if (entries[i].ptr) {
				memcpy(entries[i].ptr, start + entries[i].offset, entries[i].size);
			}
		}
		return true;
	}
}
