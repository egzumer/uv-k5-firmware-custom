
#ifdef GIT_HASH
	#define VER     GIT_HASH
#else
	#define VER     "230925"
#endif

const char Version[]      = "OEFW-"VER;
const char UART_Version[] = "UV-K5 Firmware, Open Edition, OEFW-"VER"\r\n";
