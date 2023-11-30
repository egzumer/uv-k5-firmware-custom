#ifndef DEBUGGING_H
#define DEBUGGING_H

#include "driver/uart.h"
#include "driver/bk4819.h"
#include "string.h"
#include "external/printf/printf.h"


static inline void LogUart(char * str)
{
    UART_Send(str, strlen(str));
}

static inline void LogRegUart(uint16_t reg)
{
    uint16_t regVal = BK4819_ReadRegister(reg);
    char buf[32];
    sprintf(buf, "reg%02X: %04X\n", reg, regVal);
    LogUart(buf);
}




#endif