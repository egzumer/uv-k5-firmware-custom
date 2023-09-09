/*-----------------------------------------------------------------------------
 *      Name:         CV_CML1Cache.c 
 *      Purpose:      CMSIS CORE validation tests implementation
 *-----------------------------------------------------------------------------
 *      Copyright (c) 2020 - 2021 ARM Limited. All rights reserved.
 *----------------------------------------------------------------------------*/

#include "CV_Framework.h"
#include "cmsis_cv.h"

/*-----------------------------------------------------------------------------
 *      Test implementation
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *      Test cases
 *----------------------------------------------------------------------------*/

/*=======0=========1=========2=========3=========4=========5=========6=========7=========8=========9=========0=========1====*/
void TC_CML1Cache_EnDisableICache(void) {
#ifdef __ICACHE_PRESENT
  SCB_EnableICache();
  
  ASSERT_TRUE((SCB->CCR & SCB_CCR_IC_Msk) == SCB_CCR_IC_Msk);
  
  SCB_DisableICache();

  ASSERT_TRUE((SCB->CCR & SCB_CCR_IC_Msk) == 0U);
#endif
}

/*=======0=========1=========2=========3=========4=========5=========6=========7=========8=========9=========0=========1====*/
void TC_CML1Cache_EnDisableDCache(void) {
#ifdef __DCACHE_PRESENT
  SCB_EnableDCache();

  ASSERT_TRUE((SCB->CCR & SCB_CCR_DC_Msk) == SCB_CCR_DC_Msk);

  SCB_DisableDCache();

  ASSERT_TRUE((SCB->CCR & SCB_CCR_DC_Msk) == 0U);
#endif
}

/*=======0=========1=========2=========3=========4=========5=========6=========7=========8=========9=========0=========1====*/
#ifdef __DCACHE_PRESENT
static uint32_t TC_CML1Cache_CleanDCacheByAddrWhileDisabled_Values[] = { 42U, 0U, 8U, 15U };
#endif

void TC_CML1Cache_CleanDCacheByAddrWhileDisabled(void) {
#ifdef __DCACHE_PRESENT
  SCB_DisableDCache();
  SCB_CleanDCache_by_Addr(TC_CML1Cache_CleanDCacheByAddrWhileDisabled_Values, sizeof(TC_CML1Cache_CleanDCacheByAddrWhileDisabled_Values)/sizeof(TC_CML1Cache_CleanDCacheByAddrWhileDisabled_Values[0]));
  ASSERT_TRUE((SCB->CCR & SCB_CCR_DC_Msk) == 0U);
#endif
}
