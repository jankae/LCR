#include "touch.hpp"
#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"

#define CS_LOW()			(TOUCH_CS_GPIO_Port->BSRR = TOUCH_CS_Pin<<16u)
#define CS_HIGH()			(TOUCH_CS_GPIO_Port->BSRR = TOUCH_CS_Pin)
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


extern SPI_HandleTypeDef hspi1;
extern SemaphoreHandle_t xMutexSPI1;

void Touch::Init(void) {
	CS_HIGH();
}

static uint16_t ADS7843_Read(uint8_t control) {
	/* SPI3 is also used for the SD card. Reduce speed now to properly work with
	 * ADS7843 */
	uint16_t cr1 = hspi1.Instance->CR1;
	/* Minimum CLK frequency is 2.5MHz, this could be achieved with a prescaler of 16
	 * (2.25MHz). For reliability, a prescaler of 32 is used */
	hspi1.Instance->CR1 = (hspi1.Instance->CR1 & ~SPI_BAUDRATEPRESCALER_256 )
			| SPI_BAUDRATEPRESCALER_32;
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

static bool touch_SampleADC(int16_t *rawX, int16_t *rawY, uint16_t samples) {
	uint16_t i;
	int32_t X = 0;
	int32_t Y = 0;
	int16_t Xmax = INT16_MIN, Ymax = INT16_MIN;
	int16_t Xmin = INT16_MAX, Ymin = INT16_MAX;
	for (i = 0; i < samples; i++) {
		int16_t xsample = ADS7843_Read(
		CHANNEL_X | DIFFERENTIAL | BITS12 | PD_PENIRQ);
		X += xsample;
		if (xsample > Xmax) {
			Xmax = xsample;
		}
		if (xsample < Xmin) {
			Xmin = xsample;
		}
	}
	for (i = 0; i < samples; i++) {
		int16_t ysample = ADS7843_Read(
		CHANNEL_Y | DIFFERENTIAL | BITS12 | PD_PENIRQ);
		Y += ysample;
		if (ysample > Ymax) {
			Ymax = ysample;
		}
		if (ysample < Ymin) {
			Ymin = ysample;
		}
	}
	*rawX = X / samples;
	*rawY = Y / samples;
	constexpr uint16_t maxDiff = 500;
	if (Xmax - Xmin > maxDiff || Ymax - Ymin > maxDiff) {
		return false;
	} else {
		return true;
	}
}

bool Touch::GetCoordinates(coords_t &c) {
	if (PENIRQ()) {
		uint16_t rawY, rawX;
		bool valid = false;
		/* screen is being touched */
		/* Acquire SPI resource */
		if (xSemaphoreTake(xMutexSPI1, 10)) {
			 valid = touch_SampleADC(&c.x, &c.y, 50);
			/* Release SPI resource */
			xSemaphoreGive(xMutexSPI1);
		} else {
			/* SPI is busy */
			return false;
		}
		if (!PENIRQ()) {
			/* touch has been released during measurement */
			return false;
		}
//		/* convert to screen resolution */
//		c.x = rawX * scaleX + offsetX;
//		c.y = rawY * scaleY + offsetY;
//		if(c.x < 0)
//			c.x = 0;
//		else if(c.x >= TOUCH_RESOLUTION_X)
//			c.x = TOUCH_RESOLUTION_X - 1;
//		if(c.y < 0)
//			c.y = 0;
//		else if(c.y >= TOUCH_RESOLUTION_Y)
//			c.y = TOUCH_RESOLUTION_Y - 1;
		return valid;
	} else {
		return false;
	}
}

bool Touch::SetPENCallback(exti_callback_t cb, void* ptr) {
	return exti_set_callback(TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin,
			EXTI_TYPE_FALLING, EXTI_PULL_UP, cb, ptr) == EXTI_RES_OK;
}

bool Touch::ClearPENCallback(void) {
	return exti_clear_callback(TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin)
			== EXTI_RES_OK;
}

