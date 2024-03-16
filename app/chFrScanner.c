
#include "app/app.h"
#include "app/chFrScanner.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#include "driver/system.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/bk4819.h"

int8_t            gScanStateDir;
bool              gScanKeepResult;
bool              gScanPauseMode;

#ifdef ENABLE_SCAN_RANGES
uint32_t          gScanRangeStart;
uint32_t          gScanRangeStop;
#endif

uint8_t             currentScanList = 0;
uint32_t            initialFrqOrChan;
uint8_t           	initialCROSS_BAND_RX_TX;
uint32_t            lastFoundFrqOrChan;

static void NextFreqChannel(void);
static void NextMemChannel(void);

void CHFRSCANNER_Start(const bool storeBackupSettings, const int8_t scan_direction)
{
	if (storeBackupSettings) {
		initialCROSS_BAND_RX_TX = gEeprom.CROSS_BAND_RX_TX;
		gEeprom.CROSS_BAND_RX_TX = CROSS_BAND_OFF;
		gScanKeepResult = false;
	}
	
	RADIO_SelectVfos();

	gNextMrChannel   = gRxVfo->CHANNEL_SAVE;
	//currentScanList = (currentScanList)? currentScanList : 0; // Carry on from where it was, or start at 0
	gScanStateDir    = scan_direction;

	if (IS_MR_CHANNEL(gNextMrChannel))
	{	// channel mode
		if (storeBackupSettings) {
			initialFrqOrChan = gRxVfo->CHANNEL_SAVE;
			lastFoundFrqOrChan = initialFrqOrChan;
		}
		NextMemChannel();
	}
	else
	{	// frequency mode
		if (storeBackupSettings) {
			initialFrqOrChan = gRxVfo->freq_config_RX.Frequency;
			lastFoundFrqOrChan = initialFrqOrChan;
		}
		NextFreqChannel();
	}

	gScanPauseDelayIn_10ms = scan_pause_delay_in_2_10ms;
	gScheduleScanListen    = false;
	gRxReceptionMode       = RX_MODE_NONE;
	gScanPauseMode         = false;
}




void CHFRSCANNER_ContinueScanning(void)
{
	if (IS_FREQ_CHANNEL(gNextMrChannel))
	{
		if (gCurrentFunction == FUNCTION_INCOMING)
			APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE);
		else
			NextFreqChannel();  // switch to next frequency
	}
	else
	{
		if (gCurrentCodeType == CODE_TYPE_OFF && gCurrentFunction == FUNCTION_INCOMING)
			APP_StartListening(gMonitor ? FUNCTION_MONITOR : FUNCTION_RECEIVE);
		else
			NextMemChannel();    // switch to next channel
	}
	
	gScanPauseMode      = false;
	gRxReceptionMode    = RX_MODE_NONE;
	gScheduleScanListen = false;
}




void CHFRSCANNER_Found(void)
{
	switch (gEeprom.SCAN_RESUME_MODE)
	{
		case SCAN_RESUME_TO:
			if (!gScanPauseMode)
			{
				gScanPauseDelayIn_10ms = scan_pause_delay_in_1_10ms;
				gScheduleScanListen    = false;
				gScanPauseMode         = true;
			}
			break;

		case SCAN_RESUME_CO:
		case SCAN_RESUME_SE:
			gScanPauseDelayIn_10ms = 0;
			gScheduleScanListen    = false;
			break;
	}

	if (IS_MR_CHANNEL(gRxVfo->CHANNEL_SAVE)) { //memory scan
		lastFoundFrqOrChan = gRxVfo->CHANNEL_SAVE;
	}
	else { // frequency scan
		lastFoundFrqOrChan = gRxVfo->freq_config_RX.Frequency;
	}


	gScanKeepResult = true;
}




void CHFRSCANNER_Stop(void)
{
	if(initialCROSS_BAND_RX_TX != CROSS_BAND_OFF) {
		gEeprom.CROSS_BAND_RX_TX = initialCROSS_BAND_RX_TX;
		initialCROSS_BAND_RX_TX = CROSS_BAND_OFF;
	}
	
	gScanStateDir = SCAN_OFF;

	const uint32_t chFr = gScanKeepResult ? lastFoundFrqOrChan : initialFrqOrChan;
	const bool channelChanged = chFr != initialFrqOrChan;
	if (IS_MR_CHANNEL(gNextMrChannel)) {
		gEeprom.MrChannel[gEeprom.RX_VFO]     = chFr;
		gEeprom.ScreenChannel[gEeprom.RX_VFO] = chFr;
		RADIO_ConfigureChannel(gEeprom.RX_VFO, VFO_CONFIGURE_RELOAD);

		if(channelChanged) {
			SETTINGS_SaveVfoIndices();
			gUpdateStatus = true;
		}
	}
	else {
		gRxVfo->freq_config_RX.Frequency = chFr;
		RADIO_ApplyOffset(gRxVfo);
		RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
		if(channelChanged) {
			SETTINGS_SaveChannel(gRxVfo->CHANNEL_SAVE, gEeprom.RX_VFO, gRxVfo, 1);
		}
	}

	RADIO_SetupRegisters(true);
	gUpdateDisplay = true;
}




// Get the first or last channel numerically in the specified list. For FirstOrLast, use 1 for first, or -1 for last
uint8_t CURRENT_LIST_FIRST_or_LAST_CHANNEL(uint8_t CurList, int8_t FirstOrLast)
{
	for (
			uint8_t First_Last_Chan_Val = (FirstOrLast == 1) ? MR_CHANNEL_FIRST : MR_CHANNEL_LAST;
			First_Last_Chan_Val <= MR_CHANNEL_LAST;
			First_Last_Chan_Val += FirstOrLast
		) // Loop through all possible channels
	{
		if (gMR_ChannelLists[First_Last_Chan_Val].ScanList[CurList]) // We only need to look at the array item listed
		{
			return First_Last_Chan_Val; // Match found, return it
		} 
	}
	return 0xFF; // No channels returned
}




static void NextFreqChannel(void)
{
#ifdef ENABLE_SCAN_RANGES
	if(gScanRangeStart) {
		gRxVfo->freq_config_RX.Frequency = APP_SetFreqByStepAndLimits(gRxVfo, gScanStateDir, gScanRangeStart, gScanRangeStop);
	}
	else
#endif
		gRxVfo->freq_config_RX.Frequency = APP_SetFrequencyByStep(gRxVfo, gScanStateDir);

	RADIO_ApplyOffset(gRxVfo);
	RADIO_ConfigureSquelchAndOutputPower(gRxVfo);
	RADIO_SetupRegisters(true);

#ifdef ENABLE_FASTER_CHANNEL_SCAN
	gScanPauseDelayIn_10ms = 9;   // 90ms
#else
	gScanPauseDelayIn_10ms = scan_pause_delay_in_6_10ms;
#endif

	gUpdateDisplay     = true;
}




static void NextMemChannel(void)
{

	uint8_t             PrevScanList     = currentScanList;
	uint8_t             CurrChan         = 0xFF;
	uint8_t             PrevChan         = gNextMrChannel;

	// Starting at the current list, Find the next list that is enabled (only go through each ScanList once, returning back to the original list if none found)
	for (uint8_t i = 0; i < 11; i++, currentScanList += gScanStateDir)
	{
		if (currentScanList == 0xFF) { currentScanList = 9; } // On decrement, if it reaches 0 and rolls round to 255, set to last list (9)
		else if (currentScanList > 9) { currentScanList = 0; } // On increment, if it reaches 10, roll round to the first list (0)
		if (currentScanList != PrevScanList) // No longer on the same ScanList as previous
		{
			// If Dual Watch is on, this could be the time to scan it
			// When NextMemChannel is called again, we'll continue on the newly found ScanList as it is the first found to be enabled.
			// Be careful with how the checking of the channel loop FirstChan/LastChan would work with this
		}
		if (gEeprom.SCAN_LISTS[currentScanList]) // We're on an enabled ScanList
		{
			CurrChan = RADIO_FindNextChannel(gNextMrChannel + gScanStateDir, gScanStateDir, true, currentScanList);
			uint8_t FirstChan = CURRENT_LIST_FIRST_or_LAST_CHANNEL(currentScanList, 1);
			uint8_t LastChan  = CURRENT_LIST_FIRST_or_LAST_CHANNEL(currentScanList,-1);
			if (
				//Found chan = first chan of current list AND PrevChan = last chan of current list AND we're going forward AND we haven't moved list
				(CurrChan == FirstChan && PrevChan == LastChan && gScanStateDir == 1 && i == 0)
				||
				//Found chan = last chan of current list AND PrevChan = first chan of current list AND we're going backwards AND we haven't moved list
				(CurrChan == LastChan && PrevChan == FirstChan && gScanStateDir == -1 && i == 0)
				)
			{
				// Can't use the current channel as it matches one of the criteria
				// let's loop into the next ScanList.  
				// If we get to the end again and we're back on the same list, that's ok, because i will be 10.
				CurrChan = 0xFF;
			}
			if (CurrChan != 0xFF) { break; } // Found a valid channel, break out the loop
		}
		// Do anything if it doesn't find any lists that are enabled? - Defaults to Channel 1
	}
	// SYSTEM_DelayMs(1000); // Adds a delay while scanning, useful for testing and screenshots
	if (CurrChan == 0xFF)
	{	// no valid channel found
		CurrChan = MR_CHANNEL_FIRST;
	}
	
	gNextMrChannel = CurrChan;
	if (gNextMrChannel != PrevChan)
	{
		gEeprom.MrChannel[    gEeprom.RX_VFO] = gNextMrChannel;
		gEeprom.ScreenChannel[gEeprom.RX_VFO] = gNextMrChannel;

		RADIO_ConfigureChannel(gEeprom.RX_VFO, VFO_CONFIGURE_RELOAD);
		RADIO_SetupRegisters(true);
		gUpdateStatus = true;
		gUpdateDisplay = true;
	}

#ifdef ENABLE_FASTER_CHANNEL_SCAN
	gScanPauseDelayIn_10ms = 9;  // 90ms .. <= ~60ms it misses signals (squelch response and/or PLL lock time) ?
#else
	gScanPauseDelayIn_10ms = scan_pause_delay_in_3_10ms;
#endif
}
