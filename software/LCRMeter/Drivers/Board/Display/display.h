#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <util.h>
#include "stm.h"
#include "font.h"
#include "color.h"

#define DISPLAY_WIDTH		320
#define DISPLAY_HEIGHT		240

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
	uint16_t width;
	uint16_t height;
	const uint16_t *data;
} Image_t;

void display_Init(void);
void display_SetFont(font_t f);
void display_SetForeground(color_t c);
color_t display_GetForeground(void);
void display_SetBackground(color_t c);
color_t display_GetBackground(void);
void display_Clear();
void display_Pixel(int16_t x, int16_t y, uint16_t color);
void display_HorizontalLine(int16_t x, int16_t y, uint16_t length);
void display_VerticalLine(int16_t x, int16_t y, uint16_t length);
void display_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void display_Rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void display_RectangleFull(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void display_Circle(int16_t x0, int16_t y0, uint16_t radius);
void display_CircleFull(int16_t x0, int16_t y0, uint16_t radius);
void display_Char(int16_t x, int16_t y, uint8_t c);
void display_String(int16_t x, int16_t y, const char *s);
void display_AutoCenterString(const char *s, coords_t upperLeft,
		coords_t lowerRight);
void display_Image(int16_t x, int16_t y, const Image_t *im);
void display_ImageGrayscale(int16_t x, int16_t y, const Image_t *im);
void display_SetActiveArea(uint16_t minx, uint16_t maxx, uint16_t miny, uint16_t maxy);
void display_SetDefaultArea();

#ifdef __cplusplus
}
#endif

#endif
