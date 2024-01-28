#ifdef ENABLE_MESSENGER

#include <string.h>
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/bk4819.h"
#include "external/printf/printf.h"
#include "misc.h"
#include "settings.h"
#include "radio.h"
#include "app.h"
#include "audio.h"
#include "functions.h"
#include "frequencies.h"
#include "driver/system.h"
#include "app/messenger.h"
#include "ui/ui.h"

#if defined(ENABLE_UART)
	#include "driver/uart.h"
#endif

typedef enum MsgStatus {
	READY,
  	SENDING,
  	RECEIVING,
} MsgStatus;

const uint8_t MSG_BUTTON_STATE_HELD = 1 << 1;

const uint8_t MSG_BUTTON_EVENT_SHORT =  0;
const uint8_t MSG_BUTTON_EVENT_LONG =  MSG_BUTTON_STATE_HELD;

const uint8_t MAX_MSG_LENGTH = TX_MSG_LENGTH - 1;

const uint16_t TONE2_FREQ = 0x3065; // 0x2854

#define NEXT_CHAR_DELAY 100 // 10ms tick

char T9TableLow[9][4] = { {',', '.', '?', '!'}, {'a', 'b', 'c', '\0'}, {'d', 'e', 'f', '\0'}, {'g', 'h', 'i', '\0'}, {'j', 'k', 'l', '\0'}, {'m', 'n', 'o', '\0'}, {'p', 'q', 'r', 's'}, {'t', 'u', 'v', '\0'}, {'w', 'x', 'y', 'z'} };
char T9TableUp[9][4] = { {',', '.', '?', '!'}, {'A', 'B', 'C', '\0'}, {'D', 'E', 'F', '\0'}, {'G', 'H', 'I', '\0'}, {'J', 'K', 'L', '\0'}, {'M', 'N', 'O', '\0'}, {'P', 'Q', 'R', 'S'}, {'T', 'U', 'V', '\0'}, {'W', 'X', 'Y', 'Z'} };
unsigned char numberOfLettersAssignedToKey[9] = { 4, 3, 3, 3, 3, 3, 4, 3, 4 };

char T9TableNum[9][4] = { {'1', '\0', '\0', '\0'}, {'2', '\0', '\0', '\0'}, {'3', '\0', '\0', '\0'}, {'4', '\0', '\0', '\0'}, {'5', '\0', '\0', '\0'}, {'6', '\0', '\0', '\0'}, {'7', '\0', '\0', '\0'}, {'8', '\0', '\0', '\0'}, {'9', '\0', '\0', '\0'} };
unsigned char numberOfNumsAssignedToKey[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };

char cMessage[TX_MSG_LENGTH];
char lastcMessage[TX_MSG_LENGTH];
char rxMessage[4][MAX_RX_MSG_LENGTH + 2];
unsigned char cIndex = 0;
unsigned char prevKey = 0, prevLetter = 0;
KeyboardType keyboardType = UPPERCASE;

MsgStatus msgStatus = READY;

uint8_t msgFSKBuffer[MSG_HEADER_LENGTH + MAX_RX_MSG_LENGTH];

uint16_t gErrorsDuringMSG;

uint8_t hasNewMessage = 0;

uint8_t keyTickCounter = 0;

// -----------------------------------------------------

void MSG_FSKSendData() {

	uint16_t fsk_reg59;

	// REG_51
	//
	// <15>  TxCTCSS/CDCSS   0 = disable 1 = Enable
	//
	// turn off CTCSS/CDCSS during FFSK
	const uint16_t css_val = BK4819_ReadRegister(BK4819_REG_51);
	BK4819_WriteRegister(BK4819_REG_51, 0);

	// set the FM deviation level
	const uint16_t dev_val = BK4819_ReadRegister(BK4819_REG_40);
	//UART_printf("\n BANDWIDTH : 0x%.4X", dev_val);
	{
		uint16_t deviation = 850;
		switch (gEeprom.VfoInfo[gEeprom.TX_VFO].CHANNEL_BANDWIDTH)
		{
			case BK4819_FILTER_BW_WIDE:     deviation = 1050; break;
			case BK4819_FILTER_BW_NARROW:   deviation =  850; break;
			case BK4819_FILTER_BW_NARROWER: deviation =  750; break;
		}
		//BK4819_WriteRegister(0x40, (3u << 12) | (deviation & 0xfff));
		BK4819_WriteRegister(BK4819_REG_40, (dev_val & 0xf000) | (deviation & 0xfff));
	}

	// REG_2B   0
	//
	// <15> 1 Enable CTCSS/CDCSS DC cancellation after FM Demodulation   1 = enable 0 = disable
	// <14> 1 Enable AF DC cancellation after FM Demodulation            1 = enable 0 = disable
	// <10> 0 AF RX HPF 300Hz filter     0 = enable 1 = disable
	// <9>  0 AF RX LPF 3kHz filter      0 = enable 1 = disable
	// <8>  0 AF RX de-emphasis filter   0 = enable 1 = disable
	// <2>  0 AF TX HPF 300Hz filter     0 = enable 1 = disable
	// <1>  0 AF TX LPF filter           0 = enable 1 = disable
	// <0>  0 AF TX pre-emphasis filter  0 = enable 1 = disable
	//
	// disable the 300Hz HPF and FM pre-emphasis filter
	//
	const uint16_t filt_val = BK4819_ReadRegister(BK4819_REG_2B);
	BK4819_WriteRegister(BK4819_REG_2B, (1u << 2) | (1u << 0));

	// *******************************************
	// setup the FFSK modem as best we can

	// Uses 1200/1800 Hz FSK tone frequencies 1200 bits/s
	//
	BK4819_WriteRegister(BK4819_REG_58, // 0x37C3);   // 001 101 11 11 00 001 1
		(1u << 13) |		// 1 FSK TX mode selection
							//   0 = FSK 1.2K and FSK 2.4K TX .. no tones, direct FM
							//   1 = FFSK 1200/1800 TX
							//   2 = ???
							//   3 = FFSK 1200/2400 TX
							//   4 = ???
							//   5 = NOAA SAME TX
							//   6 = ???
							//   7 = ???
							//
		(7u << 10) |		// 0 FSK RX mode selection
							//   0 = FSK 1.2K, FSK 2.4K RX and NOAA SAME RX .. no tones, direct FM
							//   1 = ???
							//   2 = ???
							//   3 = ???
							//   4 = FFSK 1200/2400 RX
							//   5 = ???
							//   6 = ???
							//   7 = FFSK 1200/1800 RX
							//
		(0u << 8) |			// 0 FSK RX gain
							//   0 ~ 3
							//
		(0u << 6) |			// 0 ???
							//   0 ~ 3
							//
		(0u << 4) |			// 0 FSK preamble type selection
							//   0 = 0xAA or 0x55 due to the MSB of FSK sync byte 0
							//   1 = ???
							//   2 = 0x55
							//   3 = 0xAA
							//
		(1u << 1) |			// 1 FSK RX bandwidth setting
							//   0 = FSK 1.2K .. no tones, direct FM
							//   1 = FFSK 1200/1800
							//   2 = NOAA SAME RX
							//   3 = ???
							//   4 = FSK 2.4K and FFSK 1200/2400
							//   5 = ???
							//   6 = ???
							//   7 = ???
							//
		(1u << 0));			// 1 FSK enable
							//   0 = disable
							//   1 = enable

	// REG_72
	//
	// <15:0> 0x2854 TONE-2 / FSK frequency control word
	//        = freq(Hz) * 10.32444 for XTAL 13M / 26M or
	//        = freq(Hz) * 10.48576 for XTAL 12.8M / 19.2M / 25.6M / 38.4M
	//
	// tone-2 = 1200Hz
	// 18583,92
	BK4819_WriteRegister(BK4819_REG_72, TONE2_FREQ);

	// REG_70
	//
	// <15>   0 TONE-1
	//        1 = enable
	//        0 = disable
	//
	// <14:8> 0 TONE-1 tuning
	//
	// <7>    0 TONE-2
	//        1 = enable
	//        0 = disable
	//
	// <6:0>  0 TONE-2 / FSK tuning
	//        0 ~ 127
	//
	// enable tone-2, set gain
	//
	BK4819_WriteRegister(BK4819_REG_70,   // 0 0000000 1 1100000
		( 0u << 15) |    // 0
		( 0u <<  8) |    // 0
		( 1u <<  7) |    // 1
		(96u <<  0));    // 96

	// REG_59
	//
	// <15>  0 TX FIFO             1 = clear
	// <14>  0 RX FIFO             1 = clear
	// <13>  0 FSK Scramble        1 = Enable
	// <12>  0 FSK RX              1 = Enable
	// <11>  0 FSK TX              1 = Enable
	// <10>  0 FSK data when RX    1 = Invert
	// <9>   0 FSK data when TX    1 = Invert
	// <8>   0 ???
	//
	// <7:4> 0 FSK preamble length selection
	//       0  =  1 byte
	//       1  =  2 bytes
	//       2  =  3 bytes
	//       15 = 16 bytes
	//
	// <3>   0 FSK sync length selection
	//       0 = 2 bytes (FSK Sync Byte 0, 1)
	//       1 = 4 bytes (FSK Sync Byte 0, 1, 2, 3)
	//
	// <2:0> 0 ???
	//
	fsk_reg59 = (0u << 15) |   // 0/1     1 = clear TX FIFO
				(0u << 14) |   // 0/1     1 = clear RX FIFO
				(0u << 13) |   // 0/1     1 = scramble
				(0u << 12) |   // 0/1     1 = enable RX
				(0u << 11) |   // 0/1     1 = enable TX
				(0u << 10) |   // 0/1     1 = invert data when RX
				(0u <<  9) |   // 0/1     1 = invert data when TX
				(0u <<  8) |   // 0/1     ???
				(15u <<  4) |   // 0 ~ 15  preamble length .. bit toggling
				(1u <<  3) |   // 0/1     sync length
				(0u <<  0);    // 0 ~ 7   ???

	// Set packet length (not including pre-amble and sync bytes that we can't seem to disable)
	BK4819_WriteRegister(BK4819_REG_5D, ((MSG_HEADER_LENGTH + MAX_RX_MSG_LENGTH) << 8));

	// REG_5A
	//
	// <15:8> 0x55 FSK Sync Byte 0 (Sync Byte 0 first, then 1,2,3)
	// <7:0>  0x55 FSK Sync Byte 1
	//
	BK4819_WriteRegister(BK4819_REG_5A, 0x5555);                   // bytes 1 & 2

	// REG_5B
	//
	// <15:8> 0x55 FSK Sync Byte 2 (Sync Byte 0 first, then 1,2,3)
	// <7:0>  0xAA FSK Sync Byte 3
	//
	BK4819_WriteRegister(BK4819_REG_5B, 0x55AA);                   // bytes 2 & 3

	// CRC setting (plus other stuff we don't know what)
	//
	// REG_5C
	//
	// <15:7> ???
	//
	// <6>    1 CRC option enable    0 = disable  1 = enable
	//
	// <5:0>  ???
	//
	// disable CRC
	//
	// NB, this also affects TX pre-amble in some way
	//
	BK4819_WriteRegister(BK4819_REG_5C, 0x5625);   // 010101100 0 100101
//		BK4819_WriteRegister(0x5C, 0xAA30);   // 101010100 0 110000
//		BK4819_WriteRegister(0x5C, 0x0030);   // 000000000 0 110000

	BK4819_WriteRegister(BK4819_REG_59, (1u << 15) | (1u << 14) | fsk_reg59);   // clear FIFO's
	BK4819_WriteRegister(BK4819_REG_59, fsk_reg59);

	SYSTEM_DelayMs(100);

	{	// load the entire packet data into the TX FIFO buffer
		const uint16_t len_buff = (MSG_HEADER_LENGTH + MAX_RX_MSG_LENGTH);
		for (size_t i = 0, j = 0; i < len_buff; i += 2, j++) {
        	BK4819_WriteRegister(BK4819_REG_5F, (msgFSKBuffer[i + 1] << 8) | msgFSKBuffer[i]);
    	}
	}

	// enable FSK TX
	BK4819_WriteRegister(BK4819_REG_59, (1u << 11) | fsk_reg59);

	{
		// allow up to 310ms for the TX to complete
		// if it takes any longer then somethings gone wrong, we shut the TX down
		unsigned int timeout = 1000 / 5;

		while (timeout-- > 0)
		{
			SYSTEM_DelayMs(5);
			if (BK4819_ReadRegister(BK4819_REG_0C) & (1u << 0))
			{	// we have interrupt flags
				BK4819_WriteRegister(BK4819_REG_02, 0);
				if (BK4819_ReadRegister(BK4819_REG_02) & BK4819_REG_02_FSK_TX_FINISHED)
					timeout = 0;       // TX is complete
			}
		}
	}
	//BK4819_WriteRegister(BK4819_REG_02, 0);

	SYSTEM_DelayMs(100);

	// disable FSK
	BK4819_WriteRegister(BK4819_REG_59, fsk_reg59);

	// restore FM deviation level
	BK4819_WriteRegister(BK4819_REG_40, dev_val);

	// restore TX/RX filtering
	BK4819_WriteRegister(BK4819_REG_2B, filt_val);

	// restore the CTCSS/CDCSS setting
	BK4819_WriteRegister(BK4819_REG_51, css_val);

}

void MSG_EnableRX(const bool enable) {

	if (enable) {
		// REG_70
		//
		// <15>    0 TONE-1
		//         1 = enable
		//         0 = disable
		//
		// <14:8>  0 TONE-1 gain
		//
		// <7>     0 TONE-2
		//         1 = enable
		//         0 = disable
		//
		// <6:0>   0 TONE-2 / FSK gain
		//         0 ~ 127
		//
		// enable tone-2, set gain

		// REG_72
		//
		// <15:0>  0x2854 TONE-2 / FSK frequency control word
		//         = freq(Hz) * 10.32444 for XTAL 13M / 26M or
		//         = freq(Hz) * 10.48576 for XTAL 12.8M / 19.2M / 25.6M / 38.4M
		//
		// tone-2 = 1200Hz

		// REG_58
		//
		// <15:13> 1 FSK TX mode selection
		//         0 = FSK 1.2K and FSK 2.4K TX .. no tones, direct FM
		//         1 = FFSK 1200 / 1800 TX
		//         2 = ???
		//         3 = FFSK 1200 / 2400 TX
		//         4 = ???
		//         5 = NOAA SAME TX
		//         6 = ???
		//         7 = ???
		//
		// <12:10> 0 FSK RX mode selection
		//         0 = FSK 1.2K, FSK 2.4K RX and NOAA SAME RX .. no tones, direct FM
		//         1 = ???
		//         2 = ???
		//         3 = ???
		//         4 = FFSK 1200 / 2400 RX
		//         5 = ???
		//         6 = ???
		//         7 = FFSK 1200 / 1800 RX
		//
		// <9:8>   0 FSK RX gain
		//         0 ~ 3
		//
		// <7:6>   0 ???
		//         0 ~ 3
		//
		// <5:4>   0 FSK preamble type selection
		//         0 = 0xAA or 0x55 due to the MSB of FSK sync byte 0
		//         1 = ???
		//         2 = 0x55
		//         3 = 0xAA
		//
		// <3:1>   1 FSK RX bandwidth setting
		//         0 = FSK 1.2K .. no tones, direct FM
		//         1 = FFSK 1200 / 1800
		//         2 = NOAA SAME RX
		//         3 = ???
		//         4 = FSK 2.4K and FFSK 1200 / 2400
		//         5 = ???
		//         6 = ???
		//         7 = ???
		//
		// <0>     1 FSK enable
		//         0 = disable
		//         1 = enable

		// REG_5C
		//
		// <15:7>  ???
		//
		// <6>     1 CRC option enable
		//         0 = disable
		//         1 = enable
		//
		// <5:0>   ???
		//
		// disable CRC

		// REG_5D
		//
		// set the packet size

		const uint16_t fsk_reg59 =
			(0u << 15) |   // 1 = clear TX FIFO
			(0u << 14) |   // 1 = clear RX FIFO
			(0u << 13) |   // 1 = scramble
			(0u << 12) |   // 1 = enable RX
			(0u << 11) |   // 1 = enable TX
			(0u << 10) |   // 1 = invert data when RX
			(0u <<  9) |   // 1 = invert data when TX
			(0u <<  8) |   // ???
			(0u <<  4) |   // 0 ~ 15 preamble length selection ..
			(1u <<  3) |   // 0/1 sync length selection
			(0u <<  0);    // 0 ~ 7  ???

		// REG_70
		//
		// <15>   0 Enable TONE1
		//        1 = Enable
		//        0 = Disable
		//
		// <14:8> 0 TONE1 tuning gain
		//        0 ~ 127
		//
		// <7>    0 Enable TONE2
		//        1 = Enable
		//        0 = Disable
		//
		// <6:0>  0 TONE2/FSK tuning gain
		//        0 ~ 127
		//
		BK4819_WriteRegister(BK4819_REG_70,
			( 0u << 15) |    // 0
			( 0u <<  8) |    // 0
			( 1u <<  7) |    // 1
			(96u <<  0));    // 96

		// Tone2 baudrate 1200
		BK4819_WriteRegister(BK4819_REG_72, TONE2_FREQ);

		BK4819_WriteRegister(BK4819_REG_58,
			(1u << 13) |		// 1 FSK TX mode selection
								//   0 = FSK 1.2K and FSK 2.4K TX .. no tones, direct FM
								//   1 = FFSK 1200 / 1800 TX
								//   2 = ???
								//   3 = FFSK 1200 / 2400 TX
								//   4 = ???
								//   5 = NOAA SAME TX
								//   6 = ???
								//   7 = ???
								//
			(7u << 10) |		// 0 FSK RX mode selection
								//   0 = FSK 1.2K, FSK 2.4K RX and NOAA SAME RX .. no tones, direct FM
								//   1 = ???
								//   2 = ???
								//   3 = ???
								//   4 = FFSK 1200 / 2400 RX
								//   5 = ???
								//   6 = ???
								//   7 = FFSK 1200 / 1800 RX
								//
			(3u << 8) |			// 0 FSK RX gain
								//   0 ~ 3
								//
			(0u << 6) |			// 0 ???
								//   0 ~ 3
								//
			(0u << 4) |			// 0 FSK preamble type selection
								//   0 = 0xAA or 0x55 due to the MSB of FSK sync byte 0
								//   1 = ???
								//   2 = 0x55
								//   3 = 0xAA
								//
			(1u << 1) |			// 1 FSK RX bandwidth setting
								//   0 = FSK 1.2K .. no tones, direct FM
								//   1 = FFSK 1200 / 1800
								//   2 = NOAA SAME RX
								//   3 = ???
								//   4 = FSK 2.4K and FFSK 1200 / 2400
								//   5 = ???
								//   6 = ???
								//   7 = ???
								//
			(1u << 0));			// 1 FSK enable
								//   0 = disable
								//   1 = enable

		// REG_5A .. bytes 0 & 1 sync pattern
		//
		// <15:8> sync byte 0
		// < 7:0> sync byte 1
		BK4819_WriteRegister(BK4819_REG_5A, 0x5555);

		// REG_5B .. bytes 2 & 3 sync pattern
		//
		// <15:8> sync byte 2
		// < 7:0> sync byte 3
		BK4819_WriteRegister(BK4819_REG_5B, 0x55AA);

		// disable CRC
		BK4819_WriteRegister(BK4819_REG_5C, 0x5625);
		// BK4819_WriteRegister(BK4819_REG_5C, 0xAA30);   // 10101010 0 0 110000

		// set the almost full threshold
		BK4819_WriteRegister(BK4819_REG_5E, (64u << 3) | (1u << 0));  // 0 ~ 127, 0 ~ 7

		{	// packet size .. sync + 14 bytes - size of a single packet

			uint16_t size = (MSG_HEADER_LENGTH + MAX_RX_MSG_LENGTH);
			// size -= (fsk_reg59 & (1u << 3)) ? 4 : 2;
			size = (((size + 1) / 2) * 2) + 2;             // round up to even, else FSK RX doesn't work
			BK4819_WriteRegister(BK4819_REG_5D, (size << 8));
		}

		// clear FIFO's then enable RX
		BK4819_WriteRegister(BK4819_REG_59, (1u << 15) | (1u << 14) | fsk_reg59);
		BK4819_WriteRegister(BK4819_REG_59, (1u << 12) | fsk_reg59);

		BK4819_WriteRegister(BK4819_REG_02, 0);

	} else {
		BK4819_WriteRegister(BK4819_REG_70, 0);
		BK4819_WriteRegister(BK4819_REG_58, 0);
	}
}


// -----------------------------------------------------

void moveUP(char (*rxMessages)[MAX_RX_MSG_LENGTH + 2]) {
    // Shift existing lines up
    strcpy(rxMessages[0], rxMessages[1]);
	strcpy(rxMessages[1], rxMessages[2]);
	strcpy(rxMessages[2], rxMessages[3]);

    // Insert the new line at the last position
	memset(rxMessages[3], 0, sizeof(rxMessages[3]));
}

void MSG_Send(const char txMessage[TX_MSG_LENGTH], bool bServiceMessage) {

	if ( msgStatus != READY ) return;

	if ( strlen(txMessage) > 0 && (TX_freq_check(gCurrentVfo->pTX->Frequency) == 0) ) {

		msgStatus = SENDING;

		RADIO_SetVfoState(VFO_STATE_NORMAL);
		BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, true);

		memset(msgFSKBuffer, 0, sizeof(msgFSKBuffer));

		// ? ToDo
		// first 20 byte sync, msg type and ID
		msgFSKBuffer[0] = 'M';
		msgFSKBuffer[1] = 'S';

		// next 20 for msg
		memcpy(msgFSKBuffer + 2, txMessage, TX_MSG_LENGTH);

		// CRC ? ToDo

		msgFSKBuffer[MAX_RX_MSG_LENGTH - 1] = '\0';
		msgFSKBuffer[MAX_RX_MSG_LENGTH + 0] = 'I';
		msgFSKBuffer[MAX_RX_MSG_LENGTH + 1] = 'D';
		msgFSKBuffer[MAX_RX_MSG_LENGTH + 2] = '0';
		msgFSKBuffer[(MSG_HEADER_LENGTH + MAX_RX_MSG_LENGTH) - 1] = '#';

		BK4819_DisableDTMF();

		//RADIO_SetTxParameters();
		FUNCTION_Select(FUNCTION_TRANSMIT);
		//SYSTEM_DelayMs(500);
		//BK4819_PlayRogerNormal(98);
		SYSTEM_DelayMs(100);

		BK4819_ExitTxMute();
		
		MSG_FSKSendData();

		SYSTEM_DelayMs(100);

		APP_EndTransmission(true);
		RADIO_SetVfoState(VFO_STATE_NORMAL);

		BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);

		MSG_EnableRX(true);
		if (!bServiceMessage) {
			moveUP(rxMessage);
			sprintf(rxMessage[3], "> %s", txMessage);
			memset(lastcMessage, 0, sizeof(lastcMessage));
			memcpy(lastcMessage, txMessage, TX_MSG_LENGTH);
			cIndex = 0;
			prevKey = 0;
			prevLetter = 0;
			memset(cMessage, 0, sizeof(cMessage));
		}
		msgStatus = READY;

	} else {
		AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
	}
}

uint8_t validate_char( uint8_t rchar ) {
	if ( (rchar == 0x1b) || (rchar >= 32 && rchar <= 127) ) {
		return rchar;
	}
	return 32;
}

void MSG_StorePacket(const uint16_t interrupt_bits) {

	//const uint16_t rx_sync_flags   = BK4819_ReadRegister(BK4819_REG_0B);

	const bool rx_sync             = (interrupt_bits & BK4819_REG_02_FSK_RX_SYNC) ? true : false;
	const bool rx_fifo_almost_full = (interrupt_bits & BK4819_REG_02_FSK_FIFO_ALMOST_FULL) ? true : false;
	const bool rx_finished         = (interrupt_bits & BK4819_REG_02_FSK_RX_FINISHED) ? true : false;

	//UART_printf("\nMSG : S%i, F%i, E%i | %i", rx_sync, rx_fifo_almost_full, rx_finished, interrupt_bits);

	if (rx_sync) {
		gFSKWriteIndex = 0;
		memset(msgFSKBuffer, 0, sizeof(msgFSKBuffer));
		msgStatus = RECEIVING;
	}

	if (rx_fifo_almost_full && msgStatus == RECEIVING) {

		const uint16_t count = BK4819_ReadRegister(BK4819_REG_5E) & (7u << 0);  // almost full threshold
		for (uint16_t i = 0; i < count; i++) {
			const uint16_t word = BK4819_ReadRegister(BK4819_REG_5F);
			if (gFSKWriteIndex < sizeof(msgFSKBuffer))
				msgFSKBuffer[gFSKWriteIndex++] = validate_char((word >> 0) & 0xff);
			if (gFSKWriteIndex < sizeof(msgFSKBuffer))
				msgFSKBuffer[gFSKWriteIndex++] = validate_char((word >> 8) & 0xff);
		}

		SYSTEM_DelayMs(10);

	}

	if (rx_finished) {

		const uint16_t fsk_reg59 = BK4819_ReadRegister(BK4819_REG_59) & ~((1u << 15) | (1u << 14) | (1u << 12) | (1u << 11));

		BK4819_WriteRegister(BK4819_REG_59, (1u << 15) | (1u << 14) | fsk_reg59);
		BK4819_WriteRegister(BK4819_REG_59, (1u << 12) | fsk_reg59);
		msgStatus = READY;

		if (gFSKWriteIndex > 2) {

			// If there's three 0x1b bytes, then it's a service message
			if (msgFSKBuffer[2] == 0x1b && msgFSKBuffer[3] == 0x1b && msgFSKBuffer[4] == 0x1b) {
			#ifdef ENABLE_MESSENGER_DELIVERY_NOTIFICATION
				// If the next 4 bytes are "RCVD", then it's a delivery notification
				if (msgFSKBuffer[5] == 'R' && msgFSKBuffer[6] == 'C' && msgFSKBuffer[7] == 'V' && msgFSKBuffer[8] == 'D') {
					UART_printf("SVC<RCPT\r\n");
					rxMessage[3][strlen(rxMessage[3])] = '+';
					gUpdateStatus = true;
					gUpdateDisplay = true;
				}
			#endif
			} else {
				moveUP(rxMessage);
				if (msgFSKBuffer[0] != 'M' || msgFSKBuffer[1] != 'S') {
					snprintf(rxMessage[3], TX_MSG_LENGTH + 2, "? unknown msg format!");
				}
				else
				{
					snprintf(rxMessage[3], TX_MSG_LENGTH + 2, "< %s", &msgFSKBuffer[2]);
				}

			#ifdef ENABLE_MESSENGER_UART
				UART_printf("SMS<%s\r\n", &msgFSKBuffer[2]);
			#endif

				if ( gScreenToDisplay != DISPLAY_MSG ) {
					hasNewMessage = 1;
					gUpdateStatus = true;
					gUpdateDisplay = true;
			#ifdef ENABLE_MESSENGER_NOTIFICATION
					gPlayMSGRing = true;
			#endif
				}
				else {
					gUpdateDisplay = true;
				}
			}
		}

		gFSKWriteIndex = 0;
		// Transmit a message to the sender that we have received the message (Unless it's a service message)
		if (msgFSKBuffer[0] == 'M' && msgFSKBuffer[1] == 'S' && msgFSKBuffer[2] != 0x1b) {
			MSG_Send("\x1b\x1b\x1bRCVD", true);
		}
	}
}

void MSG_Init() {
	memset(rxMessage, 0, sizeof(rxMessage));
	memset(cMessage, 0, sizeof(cMessage));
	memset(lastcMessage, 0, sizeof(lastcMessage));
	hasNewMessage = 0;
	msgStatus = READY;
	prevKey = 0;
    prevLetter = 0;
	cIndex = 0;
}

// ---------------------------------------------------------------------------------

void insertCharInMessage(uint8_t key) {
	if ( key == KEY_0 ) {
		if ( keyboardType == NUMERIC ) {
			cMessage[cIndex] = '0';
		} else {
			cMessage[cIndex] = ' ';
		}
		if ( cIndex < MAX_MSG_LENGTH ) {
			cIndex++;
		}
	} else if (prevKey == key)
	{
		cIndex = (cIndex > 0) ? cIndex - 1 : 0;
		if ( keyboardType == NUMERIC ) {
			cMessage[cIndex] = T9TableNum[key - 1][(++prevLetter) % numberOfNumsAssignedToKey[key - 1]];
		} else if ( keyboardType == LOWERCASE ) {
			cMessage[cIndex] = T9TableLow[key - 1][(++prevLetter) % numberOfLettersAssignedToKey[key - 1]];
		} else {
			cMessage[cIndex] = T9TableUp[key - 1][(++prevLetter) % numberOfLettersAssignedToKey[key - 1]];
		}
		if ( cIndex < MAX_MSG_LENGTH ) {
			cIndex++;
		}
	}
	else
	{
		prevLetter = 0;
		if ( cIndex >= MAX_MSG_LENGTH ) {
			cIndex = (cIndex > 0) ? cIndex - 1 : 0;
		}
		if ( keyboardType == NUMERIC ) {
			cMessage[cIndex] = T9TableNum[key - 1][prevLetter];
		} else if ( keyboardType == LOWERCASE ) {
			cMessage[cIndex] = T9TableLow[key - 1][prevLetter];
		} else {
			cMessage[cIndex] = T9TableUp[key - 1][prevLetter];
		}
		if ( cIndex < MAX_MSG_LENGTH ) {
			cIndex++;
		}

	}
	cMessage[cIndex] = '\0';
	if ( keyboardType == NUMERIC ) {
		prevKey = 0;
		prevLetter = 0;
	} else {
		prevKey = key;
	}
}

void processBackspace() {
	cIndex = (cIndex > 0) ? cIndex - 1 : 0;
	cMessage[cIndex] = '\0';
	prevKey = 0;
    prevLetter = 0;
}

void  MSG_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
	uint8_t state = bKeyPressed + 2 * bKeyHeld;

	if (state == MSG_BUTTON_EVENT_SHORT) {

		switch (Key)
		{
			case KEY_0:
			case KEY_1:
			case KEY_2:
			case KEY_3:
			case KEY_4:
			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
			case KEY_9:
				if ( keyTickCounter > NEXT_CHAR_DELAY) {
					prevKey = 0;
    				prevLetter = 0;
				}
				insertCharInMessage(Key);
				keyTickCounter = 0;
				break;
			case KEY_STAR:
				keyboardType = (KeyboardType)((keyboardType + 1) % END_TYPE_KBRD);
				break;
			case KEY_F:
				processBackspace();
				break;
			case KEY_UP:
				memset(cMessage, 0, sizeof(cMessage));
				memcpy(cMessage, lastcMessage, TX_MSG_LENGTH);
				cIndex = strlen(cMessage);
				break;
			/*case KEY_DOWN:
				break;*/
			case KEY_MENU:
				// Send message
				MSG_Send(cMessage, false);
				break;
			case KEY_EXIT:
				gRequestDisplayScreen = DISPLAY_MAIN;
				break;

			default:
				AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
				break;
		}

	} else if (state == MSG_BUTTON_EVENT_LONG) {

		switch (Key)
		{
			case KEY_F:
				MSG_Init();
				break;
			default:
				AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
				break;
		}
	}

}


#endif
