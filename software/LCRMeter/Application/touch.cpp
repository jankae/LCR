#include "touch.h"

#include "semphr.h"
#include "fatfs.h"
#include "file.hpp"
#include "Persistence.hpp"

#define CS_LOW()			(TOUCH_CS_GPIO_Port->BSRR = TOUCH_CS_Pin<<16u)
#define CS_HIGH()			(TOUCH_CS_GPIO_Port->BSRR = TOUCH_CS_Pin)
//#define DOUT_LOW()			(GPIOB->BSRR = GPIO_PIN_5<<16u)
//#define DOUT_HIGH()			(GPIOB->BSRR = GPIO_PIN_5)
//#define SCK_LOW()			(GPIOB->BSRR = GPIO_PIN_3<<16u)
//#define SCK_HIGH()			(GPIOB->BSRR = GPIO_PIN_3)
//#define DIN()				(GPIOB->IDR & GPIO_PIN_4)
#define PENIRQ()			(!(TOUCH_IRQ_GPIO_Port->IDR & TOUCH_IRQ_Pin))

/* A2-A0 bits in control word */
#define CHANNEL_X			(0x10)
#define CHANNEL_Y			(0x50)
#define CHANNEL_3			(0x20)
#define CHANNEL_4			(0x60)

/* SER/DFR bit */
#define SINGLE_ENDED		(0x04)
#define DIFFERENTIAL		(0x00)

/* Resolution */
#define BITS8				(0x80)
#define BITS12				(0x00)

/* Power down mode */
#define PD_PENIRQ			(0x00)
#define PD_NOIRQ			(0x01)
#define PD_ALWAYS_ON		(0x03)

static uint8_t calibrating = 0;

using TouchCal = struct {
	int32_t offsetX, offsetY;
	float scaleX;
	float scaleY;
};

static constexpr TouchCal DefaultCal = { .offsetX = 0, .offsetY = 0, .scaleX =
		(float) TOUCH_RESOLUTION_X / 4096, .scaleY = (float) TOUCH_RESOLUTION_Y
		/ 4096 };

static TouchCal Cal;

//const File::Entry touchCal[4] = {
//		{"xfactor", &scaleX, File::PointerType::FLOAT},
//		{"xoffset", &offsetX, File::PointerType::INT32},
//		{"yfactor", &scaleY, File::PointerType::FLOAT},
//		{"yoffset", &offsetY, File::PointerType::INT32},
//};

extern SPI_HandleTypeDef hspi1;
extern SemaphoreHandle_t xMutexSPI1;

void touch_Init(void) {
	CS_HIGH();
	Cal = DefaultCal;
	Persistence::Add(&Cal, sizeof(Cal));
}

static uint16_t ADS7843_Read(uint8_t control) {
	/* SPI3 is also used for the SD card. Reduce speed now to properly work with
	 * ADS7843 */
	uint16_t cr1 = hspi1.Instance->CR1;
	/* Minimum CLK frequency is 2.5MHz, this could be achieved with a prescaler of 32
	 * (2.25MHz). For reliability, a prescaler of 16 is used */
	hspi1.Instance->CR1 = (hspi1.Instance->CR1 & ~SPI_BAUDRATEPRESCALER_256 )
			| SPI_BAUDRATEPRESCALER_64;
	CS_LOW();
	/* highest bit in control must always be one */
	control |= 0x80;
	uint8_t read[2];
	/* transmit control word */
	HAL_SPI_Transmit(&hspi1, &control, 1, 10);
	/* read ADC result */
	uint16_t dummy = 0;
	HAL_SPI_TransmitReceive(&hspi1, (uint8_t*) &dummy, (uint8_t*) read, 2, 10);
	/* shift and mask 12-bit result */
	uint16_t res = ((uint16_t) read[0] << 8) + read[1];
	res >>= 3;
	res &= 0x0FFF;
	CS_HIGH();
	/* Reset SPI speed to previous value */
	hspi1.Instance->CR1 = cr1;
	return 4095 - res;
}

static void touch_SampleADC(uint16_t *rawX, uint16_t *rawY, uint16_t samples) {
	uint16_t i;
	uint32_t X = 0;
	uint32_t Y = 0;
	for (i = 0; i < samples; i++) {
		X += ADS7843_Read(
		CHANNEL_X | DIFFERENTIAL | BITS12 | PD_PENIRQ);
	}
	for (i = 0; i < samples; i++) {

		Y += ADS7843_Read(
		CHANNEL_Y | DIFFERENTIAL | BITS12 | PD_PENIRQ);
	}
	*rawX = X / samples;
	*rawY = Y / samples;
}

static int8_t getRaw(coords_t *c) {
	if (PENIRQ()) {
		uint16_t rawY, rawX;
		/* screen is being touched */
		/* Acquire SPI resource */
		if (xSemaphoreTake(xMutexSPI1, 10)) {
			touch_SampleADC(&rawX, &rawY, 20);
			/* Release SPI resource */
			xSemaphoreGive(xMutexSPI1);
		} else {
			/* SPI is busy */
			return -1;
		}
		if (!PENIRQ()) {
			/* touch has been released during measurement */
			return 0;
		}
		c->x = rawX;
		c->y = rawY;
		return 1;
	} else {
		return 0;
	}
}

int8_t touch_GetCoordinates(coords_t *c) {
	if(calibrating) {
		/* don't report coordinates while calibrating */
		return 0;
	}
	int8_t ret = getRaw(c);
	/* convert to screen resolution */
	c->x = constrain_int16_t(c->x * Cal.scaleX + Cal.offsetX, 0, DISPLAY_WIDTH - 1);
	c->y = constrain_int16_t(c->y * Cal.scaleY + Cal.offsetY, 0, DISPLAY_HEIGHT - 1);
	return ret;
}

//static uint8_t touch_SaveCalibration(void) {
//	if (File::Open("TOUCH.CAL", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
//		return 0;
//	}
//	File::WriteParameters(touchCal, 4);
//	File::Close();
//	return 1;
//}

//uint8_t touch_LoadCalibration(void) {
//	if (File::Open("TOUCH.CAL", FA_OPEN_EXISTING | FA_READ) != FR_OK) {
//		return 0;
//	}
//	if (File::ReadParameters(touchCal, 4) == File::ParameterResult::OK) {
//		File::Close();
//		return 1;
//	} else {
//		File::Close();
//		return 0;
//	}
//}

static coords_t GetCalibrationPoint(bool top, coords_t cross) {
	constexpr uint16_t barHeight = 20;
	int16_t y = top ? 0 : DISPLAY_HEIGHT - Font_Big.height - barHeight - 1;
	display_SetForeground(COLOR_WHITE);
	display_SetBackground(COLOR_BLACK);
	display_Clear();
	display_Line(cross.x - 10, cross.y - 10, cross.x + 10, cross.y + 10);
	display_Line(cross.x - 10, cross.y + 10, cross.x + 10, cross.y - 10);
	display_SetFont(Font_Big);
	display_String(0, y, "Press and hold X");
	display_Rectangle(0, y + Font_Big.height, DISPLAY_WIDTH - 1, y + Font_Big.height + barHeight);
	uint16_t bar = 1;
	uint16_t samples = 0;
	int32_t sumX = 0, sumY = 0;
	coords_t c;
	do {
		if(getRaw(&c)) {
			bar++;
			if(bar >= 50) {
				sumX += c.x;
				sumY += c.y;
				samples++;
			}
			display_SetForeground(COLOR_GREEN);
			display_VerticalLine(bar, y + Font_Big.height + 1, barHeight - 2);
		} else {
			bar = 1;
			samples = 0;
			sumX = sumY = 0;
			display_SetForeground(COLOR_BLACK);
			display_RectangleFull(1, y + Font_Big.height + 1, DISPLAY_WIDTH - 2,
					y + Font_Big.height + barHeight - 1);
		}
	} while (bar < DISPLAY_WIDTH - 2);
	// wait for touch to be released
	while (getRaw(&c))
		;
	coords_t ret;
	ret.x = sumX / samples;
	ret.y = sumY / samples;
	return ret;
}

//static bool SaveCalibration(void) {
//	if (File::Open("TOUCH.CAL", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
//		LOG(Log_Input, LevelError, "Failed to create calibration file");
//		return false;
//	}
//	File::WriteParameters(touchCal, 4);
//	File::Close();
//	LOG(Log_Input, LevelInfo, "Created calibration file");
//	return true;
//}


void touch_Calibrate() {
	coords_t Set1 = { .x = 20, .y = 20 };
	coords_t Set2 = { .x = DISPLAY_WIDTH - 20, .y = DISPLAY_HEIGHT - 20 };
	coords_t Meas1 = GetCalibrationPoint(false, Set1);
	coords_t Meas2 = GetCalibrationPoint(true, Set2);

	Cal.scaleX = (float) (Set2.x - Set1.x) / (Meas2.x - Meas1.x);
	Cal.scaleY = (float) (Set2.y - Set1.y) / (Meas2.y - Meas1.y);
	/* calculate offset */
	Cal.offsetX = Set1.x - Meas1.x * Cal.scaleX;
	Cal.offsetY = Set1.y - Meas1.y * Cal.scaleY;

	GUIEvent_t ev;
	ev.type = EVENT_WINDOW_CLOSE;
	GUI::SendEvent(&ev);

	if(!Persistence::Save()) {
		Dialog::MessageBox("ERROR", Font_Big,
				"Failed to save\ntouch calibration", Dialog::MsgBox::OK, nullptr,
				false);
	}
}

//void touch_Calibrate(void) {
//	if(!xSemaphoreTake(xMutexSPI3, 500)) {
//		/* SPI is busy */
//		return;
//	}
//	calibrating = 1;
//	uint8_t done = 0;
//	/* display first calibration cross */
//	display_SetBackground(COLOR_WHITE);
//	display_SetForeground(COLOR_BLACK);
//	display_Clear();
//	display_Line(0, 0, 40, 40);
//	display_Line(40, 0, 0, 40);
//	coords_t p1;
//	do {
//		/* wait for touch to be pressed */
//		while (!PENIRQ())
//			;
//		/* get raw data */
//		touch_SampleADC((uint16_t*) &p1.x, (uint16_t*) &p1.y, 1000);
//		if (p1.x <= 1000 && p1.y <= 1000)
//			done = 1;
//	} while (!done);
//	while (PENIRQ())
//		;
//	/* display second calibration cross */
//	display_Clear();
//	display_Line(DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 41,
//	DISPLAY_HEIGHT - 41);
//	display_Line(DISPLAY_WIDTH - 41, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH - 1,
//	DISPLAY_HEIGHT - 41);
//	coords_t p2;
//	done = 0;
//	do {
//		/* wait for touch to be pressed */
//		while (!PENIRQ())
//			;
//		/* get raw data */
//		touch_SampleADC((uint16_t*) &p2.x, (uint16_t*) &p2.y, 1000);
//		if (p2.x >= 3000 && p2.y >= 3000)
//			done = 1;
//	} while (!done);
//
//	/* calculate new calibration values */
//	/* calculate scale */
//	scaleX = (float) (DISPLAY_WIDTH - 40) / (p2.x - p1.x);
//	scaleY = (float) (DISPLAY_HEIGHT - 40) / (p2.y - p1.y);
//	/* calculate offset */
//	offsetX = 20 - p1.x * scaleX;
//	offsetY = 20 - p1.y * scaleY;
//
//	while(PENIRQ());
//
//	/* Release SPI resource */
//	xSemaphoreGive(xMutexSPI3);
//
//	/* Try to write calibration data to file */
//	if(touch_SaveCalibration()) {
//		printf("Wrote touch calibration file\n");
//	} else {
//		printf("Failed to create touch calibration file\n");
//	}
//
//	calibrating = 0;
//}
//
//
