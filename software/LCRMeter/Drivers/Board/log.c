#include "log.h"

#include "stm.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "freertos_hooks.h"

/* Automatically build register and function names based on USART selection */
#define USART_M2(y) 		USART ## y
#define USART_M1(y)  		USART_M2(y)
#define USART_BASE			USART_M1(LOG_USART)

#define HANDLER_M2(x) 		USART ## x ## _IRQHandler
#define HANDLER_M1(x)  		HANDLER_M2(x)
#define HANDLER				HANDLER_M1(LOG_USART)

#define NVIC_ISR_M2(x) 		USART ## x ## _IRQn
#define NVIC_ISR_M1(x)  	NVIC_ISR_M2(x)
#define NVIC_ISR			NVIC_ISR_M1(LOG_USART)

#define CLK_ENABLE_M2(x) 	__HAL_RCC_USART ## x ## _CLK_ENABLE()
#define CLK_ENABLE_M1(x)  	CLK_ENABLE_M2(x)
#define CLK_ENABLE()		CLK_ENABLE_M1(LOG_USART)

#define CLK_DISABLE_M2(x) 	__HAL_RCC_USART ## x ## _CLK_DISABLE()
#define CLK_DISABLE_M1(x)  	CLK_DISABLE_M2(x)
#define CLK_DISABLE()		CLK_DISABLE_M1(LOG_USART)


#define MAX_LINE_LENGTH		256

#ifdef USART_SR_TXE
#define USART_ISR_REG		SR
#define USART_RXNE			USART_SR_RXNE
#define USART_TXE			USART_SR_TXE
#define USART_TC			USART_SR_TC
#define USART_READ			DR
#define USART_WRITE			DR
#else
#define USART_ISR_REG		ISR
#define USART_RXNE			USART_ISR_RXNE
#define USART_TXE			USART_ISR_TXE
#define USART_TC			USART_ISR_TC
#define USART_READ			RDR
#define USART_WRITE			TDR
#endif

static char fifo[LOG_SENDBUF_LENGTH + MAX_LINE_LENGTH];
static uint16_t fifo_write, fifo_read;

#ifdef LOG_USE_MUTEXES
static StaticSemaphore_t xMutex;
static SemaphoreHandle_t mutex;
#endif

static const char lvl_strings[][4] = {
	"DBG",
	"INF",
	"WRN",
	"ERR",
	"CRT",
};

#define INC_FIFO_POS(pos, inc) do { pos = (pos + inc) % LOG_SENDBUF_LENGTH; } while(0)

static uint16_t fifo_space() {
	uint16_t used;
	if(fifo_write >= fifo_read) {
		used = fifo_write - fifo_read;
	} else {
		used = fifo_write - fifo_read + LOG_SENDBUF_LENGTH;
	}
	return LOG_SENDBUF_LENGTH - used - 1;
}

void log_init() {
	fifo_write = 0;
	fifo_read = 0;
#ifdef LOG_USE_MUTEXES
	mutex = xSemaphoreCreateMutexStatic(&xMutex);
#endif

	/* USART interrupt Init */
	HAL_NVIC_SetPriority(NVIC_ISR, 0, 0);
	HAL_NVIC_EnableIRQ(NVIC_ISR);
}

void log_write(const char *module, uint8_t level, const char *fmt, ...) {
	int written = 0;
	va_list args;
	va_start(args, fmt);
	uint8_t lvl = 31 - __builtin_clz(level);
#ifdef LOG_USE_MUTEXES
	if (!stm_in_interrupt()) {
		xSemaphoreTake(mutex, portMAX_DELAY);
	}
#endif
	written = snprintf(&fifo[fifo_write], MAX_LINE_LENGTH, "%05lu [%6.6s,%s]: ",
			HAL_GetTick(), module + 4, lvl_strings[lvl]);
	written += vsnprintf(&fifo[fifo_write + written], MAX_LINE_LENGTH - written,
			fmt, args);
	written += snprintf(&fifo[fifo_write + written], MAX_LINE_LENGTH - written,
			"\r\n");
	// check if line still fits into ring buffer
#ifdef LOG_BLOCKING
	while (written > fifo_space()) {
		HAL_Delay(1);
	}
#else
	if (written > fifo_space()) {
		// unable to fit line, skip
#ifdef LOG_USE_MUTEXES
		if (!stm_in_interrupt()) {
			xSemaphoreGive(mutex);
		}
#endif
		return;
	}
#endif
	int16_t overflow = (fifo_write + written) - LOG_SENDBUF_LENGTH;
	if (overflow > 0) {
		// printf wrote over the end of the ring buffer -> wrap around
		memmove(&fifo[0], &fifo[LOG_SENDBUF_LENGTH], overflow);
	}
	INC_FIFO_POS(fifo_write, written);
	// enable interrupt
	CLK_ENABLE();
	if (!(USART_BASE->CR1 & USART_CR1_TCIE)) {
		// log buffer was empty -> prevent processor from entering stop mode
		PD_PREVENT_STOP();
	}
	USART_BASE->CR1 |= USART_CR1_TXEIE | USART_CR1_TCIE;
#ifdef LOG_USE_MUTEXES
	if (!stm_in_interrupt()) {
		xSemaphoreGive(mutex);
	}
#endif
#ifdef LOG_BLOCKING
	log_flush();
#endif
}

void log_flush() {
	while (USART_BASE->CR1 & USART_CR1_TCIE)
		;
}

void log_force(const char *fmt, ...) {
	CLK_ENABLE();
	// disable ISR
	USART_BASE->CR1 &= ~(USART_CR1_TXEIE|USART_CR1_TCIE);

	int written = 0;
	va_list args;
	va_start(args, fmt);
	written += vsnprintf(&fifo[written], MAX_LINE_LENGTH - written,
			fmt, args);
	written += snprintf(&fifo[written], MAX_LINE_LENGTH - written,
			"\r\n");

	for (uint16_t i = 0; i < written; i++) {
		while (!(USART_BASE->USART_ISR_REG & USART_TXE));
		USART_BASE->USART_WRITE = fifo[i];
	}
}

/* Implemented directly here for speed reasons. Disable interrupt in CubeMX! */
void HANDLER(void) {
	if (USART_BASE->USART_ISR_REG & USART_TC) {
		// clear flag
		USART_BASE->USART_ISR_REG &= ~USART_TC;
		if (!(USART_BASE->CR1 & USART_CR1_TXEIE)) {
			USART_BASE->CR1 &= ~USART_CR1_TCIE;
			CLK_DISABLE();
			// sent complete buffer, stop mode is allowed again
			PD_ALLOW_STOP();
		}
	}
	if ((USART_BASE->CR1 & USART_CR1_TXEIE)
			&& (USART_BASE->USART_ISR_REG & USART_TXE)) {
		USART_BASE->USART_WRITE = fifo[fifo_read];
		INC_FIFO_POS(fifo_read, 1);
		if (fifo_read == fifo_write) {
			// all done, disable interrupt
			USART_BASE->CR1 &= ~USART_CR1_TXEIE;
		}
	}
}

#if configCHECK_FOR_STACK_OVERFLOW > 0
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName) {
	log_write("    FreeRTOS", LevelCrit, "Stack overflow in task: %s",
			pcTaskName);
	log_flush();
	__BKPT();
}
#endif
