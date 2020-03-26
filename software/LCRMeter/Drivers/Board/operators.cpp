#include "FreeRTOS.h"
#include <cstdio>
#include "log.h"

void * operator new(size_t size)
{
	void *ptr = pvPortMalloc(size);
	LOG(Log_System, LevelDebug, "New: allocating %d bytes: %p", size, ptr);
	return ptr;
}

void * operator new[](size_t size)
{
	void *ptr = pvPortMalloc(size);
	LOG(Log_System, LevelDebug, "New: allocating %d bytes: %p", size, ptr);
	return ptr;
}

void operator delete(void* ptr)
{
	LOG(Log_System, LevelDebug, "Delete: freeing pointer: %p", ptr);
    vPortFree(ptr);
}

void operator delete[](void* ptr)
{
	LOG(Log_System, LevelDebug, "Delete: freeing pointer: %p", ptr);
    vPortFree(ptr);
}

extern "C" void __cxa_pure_virtual() {
	LOG(Log_System, LevelCrit, "Pure virtual");
	while (1);
}

