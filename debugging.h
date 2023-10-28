#ifndef DEBUGGING_H
#define DEBUGGING_H

#include "driver/uart.h"
#include "string.h"
#include "external/printf/printf.h"


static inline void LogUart(char * str)
{
    UART_Send(str, strlen(str));
}





#endif