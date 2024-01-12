#ifndef DEBUGGING_H
#define DEBUGGING_H

#ifdef ENABLE_UART

#include "driver/uart.h"
#include "driver/bk4819.h"
#include "string.h"
#include "external/printf/printf.h"
#include "am_fix.h"

static inline void LogUart(const char *const str)
{
	UART_Send(str, strlen(str));
}

static inline void LogUartf(const char* format, ...)
{
	char buffer[128];
	va_list va;
	va_start(va, format);
	vsnprintf(buffer, (size_t)-1, format, va);
	va_end(va);
	UART_Send(buffer, strlen(buffer));
}

static inline void LogRegUart(uint16_t reg)
{
	uint16_t regVal = BK4819_ReadRegister(reg);
	char buf[32];
	sprintf(buf, "reg%02X: %04X\n", reg, regVal);
	LogUart(buf);
}

static inline void LogPrint()
{
	uint16_t rssi = BK4819_GetRSSI();
	uint16_t reg7e = BK4819_ReadRegister(0x7E);
	char buf[32];
	sprintf(buf, "reg7E: %d  %2d  %6d  %2d  %d   rssi: %d\n", (reg7e >> 15),
		(reg7e >> 12) & 0b111, (reg7e >> 5) & 0b1111111,
		(reg7e >> 2) & 0b111, (reg7e >> 0) & 0b11, rssi);
	LogUart(buf);
}

#endif

#endif
