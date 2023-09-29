
#define ONE_OF_ELEVEN_VER

#ifdef GIT_HASH
	#define VER     GIT_HASH
#else
	#define VER     "230929"
#endif

#ifndef ONE_OF_ELEVEN_VER
	const char Version[]      = "OEFW-"VER;
	const char UART_Version[] = "UV-K5 Firmware, Open Edition, OEFW-"VER"\r\n";
#else
	const char Version[]      = "1o11-"VER;
	const char UART_Version[] = "UV-K5 Firmware, Open Edition, 1o11-"VER"\r\n";
#endif
