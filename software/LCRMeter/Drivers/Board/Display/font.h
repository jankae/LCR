#ifndef GUI_FONT_H_
#define GUI_FONT_H_

#include <stdint.h>

typedef struct {
	const uint8_t *data;
    uint8_t width;
    uint8_t height;
} font_t;

extern const font_t Font_Small;
extern const font_t Font_Medium;
extern const font_t Font_Big;

#endif
