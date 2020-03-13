#include "Config.hpp"
#include "file.hpp"
#include "log.h"

using ConfigEntry = struct configEntry {
	Config::WriteFunc write;
	Config::ReadFunc read;
	void *ptr;
	configEntry *next;
	uint32_t index;
};

static ConfigEntry *first;
static uint32_t cnt;

void Config::Init() {
	first = nullptr;
	cnt = 0;
}

int Config::AddParseFunctions(WriteFunc write, ReadFunc read, void *ptr) {
	cnt++;
	ConfigEntry *add = new ConfigEntry;
	add->write = write;
	add->read = read;
	add->ptr = ptr;
	add->next = nullptr;
	add->index = cnt;
	if (!first) {
		first = add;
	} else {
		// find end of config entries
		ConfigEntry *last = first;
		while (last->next) {
			last = last->next;
		}
		last->next = add;
	}
	return cnt;
}

bool Config::RemoveParseFunctions(int index) {
	if (index < 0) {
		return false;
	}
	ConfigEntry **ptrTo = &first;
	bool removed = false;
	while(*ptrTo) {
		if((*ptrTo)->index == index) {
			// remove this
			ConfigEntry *toDelete = *ptrTo;
			*ptrTo = toDelete->next;
			delete toDelete;
			removed = true;
			break;
		} else {
			ptrTo = &(*ptrTo)->next;
		}
	}
	return removed;
}

bool Config::Store(const char* filename) {
	if (File::Open(filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
		LOG(Log_Config, LevelError, "Failed to\ncreate file");
		return false;
	}
	ConfigEntry *entry = first;
	bool success = true;
	while (entry) {
		if (entry->write) {
			if (!entry->write(entry->ptr)) {
				LOG(Log_Config, LevelError, "Config write function failed");
				success = false;
				break;
			}
			File::Write("\n");
		}
		entry = entry->next;
	}
	File::Close();
	return success;
}

bool Config::Load(const char* filename) {
	if (File::Open(filename, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
		LOG(Log_Config, LevelError, "Failed to\nopen file");
		return false;
	}
	ConfigEntry *entry = first;
	bool success = true;
	while (entry) {
		if (entry->read) {
			if (!entry->read(entry->ptr)) {
				LOG(Log_Config, LevelError, "Config read function failed");
				success = false;
				break;
			}
		}
		entry = entry->next;
	}
	File::Close();
	return success;
}
