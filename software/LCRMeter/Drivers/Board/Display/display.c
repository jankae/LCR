#include <string.h>
#include <stdlib.h>
#include "display.h"
#include "main.h"
#include "log.h"

#define RST_HIGH()			(DISP_RST_GPIO_Port->BSRR = DISP_RST_Pin)
#define RST_LOW()			(DISP_RST_GPIO_Port->BSRR = DISP_RST_Pin<<16u)
#define CS_HIGH()			(DISP_CS_GPIO_Port->BSRR = DISP_CS_Pin)
#define CS_LOW()			(DISP_CS_GPIO_Port->BSRR = DISP_CS_Pin<<16u)
#define RD_HIGH()			(DISP_RD_GPIO_Port->BSRR = DISP_RD_Pin)
#define RD_LOW()			(DISP_RD_GPIO_Port->BSRR = DISP_RD_Pin<<16u)
#define WR_HIGH()			(DISP_WR_GPIO_Port->BSRR = DISP_WR_Pin)
#define WR_LOW()			(DISP_WR_GPIO_Port->BSRR = DISP_WR_Pin<<16u)
#define RS_HIGH()			(DISP_RS_GPIO_Port->BSRR = DISP_RS_Pin)
#define RS_LOW()			(DISP_RS_GPIO_Port->BSRR = DISP_RS_Pin<<16u)

color_t foreground;
color_t background;
font_t font;

typedef struct  {
	uint16_t minX;
	uint16_t maxX;
	uint16_t minY;
	uint16_t maxY;
} activeArea_t;

static activeArea_t active;

static inline void setData(uint16_t data) {
	uint32_t buf = data;
//	asm("rev %0, %0\n\t"
//		"rbit %0, %0"
//		: "=r" (buf)
//		: "r" (buf));
	GPIOE->ODR = buf;
}

static inline void selectRegister(uint8_t reg) {
	RS_LOW();
	setData(reg);
	WR_LOW();
	CS_LOW();
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	CS_HIGH();
	WR_HIGH();
}

static inline void writeData(uint16_t data) {
	RS_HIGH();
	setData(data);
	WR_LOW();
	CS_LOW();
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	CS_HIGH();
	WR_HIGH();
}

uint16_t readData(void) {
	/* configure data pins as inputs */
	GPIO_InitTypeDef GPIO_InitStruct;
	/*Configure GPIO pins : PD8 PD9 PD10 PD11
	 PD12 PD13 PD14 PD15
	 PD0 PD1 PD2 PD3
	 PD4 PD5 PD6 PD7 */
	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
			| GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_0
			| GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
			| GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/* Read cycle */
	RS_LOW();
	HAL_Delay(2);
	RD_LOW();
	HAL_Delay(2);
	CS_LOW();
	HAL_Delay(2);
	uint16_t data = GPIOE->IDR;
	CS_HIGH();
	HAL_Delay(2);
	RD_HIGH();
	HAL_Delay(2);

	/* configure data pins as outputs */
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	return data;
}

void writeRegister(uint8_t reg, uint16_t data) {
	selectRegister(reg);
	writeData(data);
}

uint16_t readRegister(uint8_t reg) {
	selectRegister(reg);
	return readData();
}

static inline void setYStartStop(uint16_t start, uint16_t stop) {
	writeRegister(0x44, (stop << 8) + start);
}

static inline void setXStart(uint16_t start) {
	writeRegister(0x45, start);
}

static inline void setXStop(uint16_t stop) {
	writeRegister(0x46, stop);
}

void setXY(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	/* convert to landscape mode */
	y0 = DISPLAY_HEIGHT - y0 - 1;
	y1 = DISPLAY_HEIGHT - y1 - 1;
	/* set start and stop values */
	setYStartStop(y1, y0);
	setXStart(x0);
	setXStop(x1);
	/* start in top left corner */
	writeRegister(0x4e, y0);
	writeRegister(0x4f, x0);
	selectRegister(0x22);
}

void SSD1289_Init(void) {
	RST_HIGH();
	HAL_Delay(5);
	RST_LOW();
	HAL_Delay(15);
	RST_HIGH();
	HAL_Delay(15);
	CS_LOW();
	RD_HIGH();
	WR_HIGH();


	/* enable oscillator */
	writeRegister(0x00, 0x0001);
	/* Step-up cycle 8 color = fosc/4, step-up factor = +5/-4, step-up cycle 262k color = fosc/4, Op-amp power medium to large */
	writeRegister(0x03, 0xA8A4);
	/* VCIX2 voltage = 5.1V */
	writeRegister(0x0C, 0x0000);
	/* VLCD63 = Vref * 2.5 */
	writeRegister(0x0D, 0x080C);
	/* VCOM = VLCD63 * 0.93 */
	writeRegister(0x0E, 0x2B00);
	/* VCOMH = VLCD63 * 0.9 */
	writeRegister(0x1E, 0x00B7);
	/* REV = 1, CAD = 0, BGR = 1, TB = 1, 319 lines */
	writeRegister(0x01, 0x2B3F);
	/* B/C = 1, EOR = 1 */
	writeRegister(0x02, 0x0600);
	/* disable sleep mode */
	writeRegister(0x10, 0x0000);
	/* 65k color mode, automatic increase of address counter */
	writeRegister(0x11, 0x6068);
	/* clear compare registers */
	writeRegister(0x05, 0x0000);
	writeRegister(0x06, 0x0000);
	/* pixels per line = 240, front porch = 30 */
	writeRegister(0x16, 0xEF1C);
	/* VFP = 0, VBP = 3 */
	writeRegister(0x17, 0x0003);

	writeRegister(0x07, 0x0233);
	writeRegister(0x0B, 0x0000);
	writeRegister(0x0F, 0x0000);
	writeRegister(0x41, 0x0000);
	writeRegister(0x42, 0x0000);
	writeRegister(0x48, 0x0000);
	writeRegister(0x49, 0x013F);
	writeRegister(0x4A, 0x0000);
	writeRegister(0x4B, 0x0000);
	writeRegister(0x44, 0xEF00);
	writeRegister(0x45, 0x0000);
	writeRegister(0x46, 0x013F);
	writeRegister(0x30, 0x0707);
	writeRegister(0x31, 0x0204);
	writeRegister(0x32, 0x0204);
	writeRegister(0x33, 0x0502);
	writeRegister(0x34, 0x0507);
	writeRegister(0x35, 0x0204);
	writeRegister(0x36, 0x0204);
	writeRegister(0x37, 0x0502);
	writeRegister(0x3A, 0x0302);
	writeRegister(0x3B, 0x0302);
	writeRegister(0x23, 0x0000);
	writeRegister(0x24, 0x0000);
	writeRegister(0x25, 0x8000);
	writeRegister(0x4f, 0x0000);
	writeRegister(0x4e, 0x0000);

	selectRegister(0x22);
}

void display_Init(void){
	SSD1289_Init();
	background = COLOR_BG_DEFAULT;
	foreground = COLOR_FG_DEFAULT;
	font = Font_Big;
	display_SetDefaultArea();
}

void display_SetFont(font_t f) {
	font = f;
}

void display_SetForeground(color_t c) {
	foreground = c;
}

color_t display_GetForeground(void) {
	return foreground;
}

void display_SetBackground(color_t c) {
	background = c;
}

color_t display_GetBackground(void) {
	return background;
}

void display_Clear() {
	setXY(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
	uint32_t i = DISPLAY_WIDTH * DISPLAY_HEIGHT;
//	for (uint16_t j = 0x0001; j > 0; j <<= 1) {
//		i = 500;
//		for (; i > 0; i--) {
//			writeData(j);
//		}
//	}
//	i = 1000;
	for (; i > 0; i--) {
		writeData(background);
	}
//	i = 1000;
//	for (; i > 0; i--) {
//		writeData(COLOR_GREEN);
//	}
//	setXY(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
//	i = 10;
//	for (; i > 0; i--) {
//		uint16_t data = readData();
//		LOG(Log_App, LevelWarn, "%u: 0x%04x", i, data);
//	}
}

void display_Pixel(int16_t x, int16_t y, uint16_t color) {
	if (x >= active.minX && x <= active.maxX && y >= active.minY
			&& y <= active.maxY) {
		setXY(x, y, x, y);
		writeData(color);
	}
}

void display_HorizontalLine(int16_t x, int16_t y, uint16_t length) {
	if (y >= active.minY && y <= active.maxY && x <= active.maxX && x + length > active.minX) {
		if (x < active.minX) {
			length -= active.minX - x;
			x = active.minX;
		}
		if (x + length - 1 > active.maxX) {
			length = active.maxX - x + 1;
		}
		setXY(x, y, x + length - 1, y);
		for (; length > 0; length--) {
			writeData(foreground);
		}
	}
}

void display_VerticalLine(int16_t x, int16_t y, uint16_t length) {
	if (x >= active.minX && x <= active.maxX && y <= active.maxY && y + length > active.minY) {
		if (y < active.minY) {
			length -= active.minY - y;
			y = active.minY;
		}
		if (y + length - 1 > active.maxY) {
			length = active.maxY - y + 1;
		}
		setXY(x, y, x, y + length - 1);
		for (; length > 0; length--) {
			writeData(foreground);
		}
	}
}

void display_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
	uint16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	uint16_t dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int16_t err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		display_Pixel(x0, y0, foreground);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

void display_Rectangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
	display_VerticalLine(x0, y0, y1 - y0);
	display_VerticalLine(x1, y0 + 1, y1 - y0);
	display_HorizontalLine(x0 + 1, y0, x1 - x0);
	display_HorizontalLine(x0, y1, x1 - x0);
}

void display_RectangleFull(int16_t x0, int16_t y0, int16_t x1, int16_t y1){
	if(x0 > active.maxX || y0 > active.maxY || x1 < active.minX || y1 < active.minY) {
		/* completely out of active area, skip */
		return;
	}
	if (x0 < active.minX) {
		x0 = active.minX;
	}
	if (x1 > active.maxX) {
		x1 = active.maxX;
	}
	if (y0 < active.minY) {
		y0 = active.minY;
	}
	if (y1 > active.maxY) {
		y1 = active.maxY;
	}
	setXY(x0, y0, x1, y1);
	uint32_t i = (x1 - x0 + 1) * (y1 - y0 + 1);
	for (; i > 0; i--) {
		writeData(foreground);
	}
}

void display_Circle(int16_t x0, int16_t y0, uint16_t radius)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
    	display_Pixel(x0 + x, y0 + y, foreground);
    	display_Pixel(x0 + y, y0 + x, foreground);
    	display_Pixel(x0 - y, y0 + x, foreground);
    	display_Pixel(x0 - x, y0 + y, foreground);
    	display_Pixel(x0 - x, y0 - y, foreground);
    	display_Pixel(x0 - y, y0 - x, foreground);
    	display_Pixel(x0 + y, y0 - x, foreground);
    	display_Pixel(x0 + x, y0 - y, foreground);

        y += 1;
        if (err <= 0)
        {
            err += 2*y + 1;
        } else {
            x -= 1;
            err += 2 * (y - x) + 1;
        }
    }
}

void display_CircleFull(int16_t x0, int16_t y0, uint16_t radius) {
	for (int y = -radius; y <= radius; y++)
		for (int x = -radius; x <= radius; x++)
			if (x * x + y * y <= radius * radius)
				display_Pixel(x0 + x, y0 + y, foreground);
}


void display_Char(int16_t x, int16_t y, uint8_t c) {
	if(x > active.maxX || y > active.maxY || x + font.width < active.minX || y + font.height < active.minY) {
		/* Character completely out of active area, skip */
		return;
	}
	uint16_t skipLeft = 0, skipRight = 0, skipTop = 0, skipBottom = 0;
	if (x < active.minX)
		skipLeft = active.minX - x;
	if (y < active.minY)
		skipTop = active.minY - y;
	if (x + font.width > active.maxX + 1)
		skipRight = x + font.width - active.maxX - 1;
	if (y + font.height > active.maxY + 1)
		skipBottom = y + font.height - active.maxY - 1;

	setXY(x + skipLeft, y + skipTop, x + font.width - 1 - skipRight, y + font.height - 1 - skipBottom);
	/* number of bytes in font per row */
	uint8_t yInc = (font.width - 1) / 8 + 1;
	const uint8_t *charIndex = font.data + c * yInc * font.height;
	uint8_t i, j;
	uint8_t startMask = 0x80 >> (yInc * 8 - font.width);
	for (i = skipTop; i < font.height - skipBottom; i++) {
		uint8_t offset = (i + 1) * yInc - 1;
		uint8_t bitMask = startMask;
		for (j = 0; j < font.width - skipRight; j++) {
			if (j >= skipLeft) {
				uint16_t color;
				if (charIndex[offset] & bitMask) {
					color = foreground;
				} else {
					color = background;
				}
				writeData(color);
			}
			bitMask >>= 1;
			if (!bitMask) {
				bitMask = 0x80;
				offset--;
			}
		}
	}
}

void display_CharRotated(int16_t x, int16_t y, uint8_t c) {
	if(x > active.maxX || y > active.maxY || x + font.height < active.minX || y + font.width < active.minY) {
		/* Character completely out of active area, skip */
		return;
	}
	uint16_t skipLeft = 0, skipRight = 0, skipTop = 0, skipBottom = 0;
	if (x < active.minX)
		skipLeft = active.minX - x;
	if (y < active.minY)
		skipTop = active.minY - y;
	if (x + font.height > active.maxX + 1)
		skipRight = x + font.height - active.maxX - 1;
	if (y + font.width > active.maxY + 1)
		skipBottom = y + font.width - active.maxY - 1;

	setXY(x + skipLeft, y + skipTop, x + font.height - 1 - skipRight, y + font.width - 1 - skipBottom);
	/* number of bytes in font per row */
	uint8_t yInc = (font.width - 1) / 8 + 1;
	const uint8_t *charIndex = font.data + c * yInc * font.height;
	uint8_t i, j;
	int8_t offset_i = 1 - yInc;
	uint8_t bitMask = 0x01;
	for (i = skipTop; i < font.width - skipBottom; i++) {
		for (j = 0; j < font.height - skipRight; j++) {
			if (j >= skipLeft) {
				uint16_t color;
				uint8_t offset = (j + 1) * yInc - 1;
				if (charIndex[offset + offset_i] & bitMask) {
					color = foreground;
				} else {
					color = background;
				}
				writeData(color);
			}
		}
		bitMask <<= 1;
		if (!bitMask) {
			bitMask = 0x01;
			offset_i++;
		}
	}
}

void display_String(int16_t x, int16_t y, const char *s) {
	while (*s) {
		display_Char(x, y, *s++);
		x += font.width;
		if (x > active.maxX)
			break;
	}
}

void display_StringRotated(int16_t x, int16_t y, const char *s) {
	while (*s) {
		display_CharRotated(x, y - font.width, *s++);
		y -= font.width;
		if (y < active.minY)
			break;
	}
}

void display_AutoCenterString(const char* s, coords_t upperLeft,
		coords_t lowerRight) {
	uint8_t lines = 1;
	uint16_t maxLineLength = 0;
	const char *start = s;
	const char *linebreak;
	while(linebreak = strchr(start, '\n')) {
		uint16_t segmentLength = linebreak - start;
		if (segmentLength > maxLineLength) {
			maxLineLength = segmentLength;
		}
		start = linebreak + 1;
		lines++;
	}
	// check last string segment
	if(strlen(start) > maxLineLength) {
		maxLineLength = strlen(start);
	}
	uint16_t lenX = lowerRight.x - upperLeft.x + 1;
	uint16_t lenY = lowerRight.y - upperLeft.y + 1;
	font_t font = Font_Big;
	if (lenX < maxLineLength * Font_Medium.width
			|| lenY < Font_Medium.height * lines) {
		font = Font_Small;
	} else if (lenX < maxLineLength * Font_Big.width
			|| lenY < Font_Big.height * lines) {
		font = Font_Medium;
	}
	display_SetFont(font);
	uint16_t shiftY = (lenY - font.height * lines) / 2;
	start = s;
	do {
		linebreak = strchr(start, '\n');
		uint8_t len;
		char strbuf[maxLineLength + 1];
		if (linebreak) {
			len = linebreak - start;
		} else {
			len = strlen(start);
		}
		strncpy(strbuf, start, len);
		strbuf[len] = 0;
		uint16_t shiftX = (lenX - len * font.width) / 2;
		display_String(upperLeft.x + shiftX, upperLeft.y + shiftY, strbuf);
		start = linebreak + 1;
		shiftY += font.height;
	} while (linebreak);
}

void display_Image(int16_t x, int16_t y, const Image_t *im) {
//	usb_DisplayCommand(0, x);
//	usb_DisplayCommand(1, y);
//	usb_DisplayCommand(2, x + im->width - 1);
//	usb_DisplayCommand(3, y + im->height - 1);
	setXY(x, y, x + im->width - 1, y + im->height - 1);
	uint32_t i = im->width * im->height;
	const uint16_t *ptr = im->data;
	for (; i > 0; i--) {
		writeData(*ptr++);
//		usb_DisplayCommand(4, *ptr++);
	}
}
void display_ImageGrayscale(int16_t x, int16_t y, const Image_t *im){
	//	usb_DisplayCommand(0, x);
	//	usb_DisplayCommand(1, y);
	//	usb_DisplayCommand(2, x + im->width - 1);
	//	usb_DisplayCommand(3, y + im->height - 1);
	setXY(x, y, x + im->width - 1, y + im->height - 1);
	uint32_t i = im->width * im->height;
	const uint16_t *ptr = im->data;
	for (; i > 0; i--) {
		/* convert to grayscale */
		uint16_t gray = COLOR_R(*ptr) + COLOR_G(*ptr) + COLOR_B(*ptr);
		gray /= 3;
		writeData(COLOR(gray, gray, gray));
//		usb_DisplayCommand(4, COLOR(gray, gray, gray));
		ptr++;
	}
}

void display_SetActiveArea(uint16_t minx, uint16_t maxx, uint16_t miny,
		uint16_t maxy) {
	active.minX = minx;
	active.maxX = maxx;
	active.minY = miny;
	active.maxY = maxy;
}

void display_SetDefaultArea() {
	active.minX = 0;
	active.maxX = DISPLAY_WIDTH - 1;
	active.minY = 0;
	active.maxY = DISPLAY_HEIGHT - 1;
}
