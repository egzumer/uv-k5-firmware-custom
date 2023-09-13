
#ifdef GIT_HASH
	const char Version[]        = "OEFW-" GIT_HASH;
	const char UART_Version[45] = "UV-K5 Firmware, Open Edition, OEFW-"GIT_HASH"\r\n";
#else
	const char Version[]        = "OEFW-230913";
	const char UART_Version[45] = "UV-K5 Firmware, Open Edition, OEFW-230913\r\n";
#endif
