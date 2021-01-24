#include <zephyr.h>
#include <logging/log.h>

#include "led_matrix.h"
#include "usbdev.h"

LOG_MODULE_REGISTER(main);

static void enable_crs()
{
#if CONFIG_CLOCK_STM32_PLL_SRC_HSI
    __HAL_RCC_CRS_CLK_ENABLE();
    LL_CRS_DeInit();
    LL_CRS_SetSyncDivider(LL_CRS_SYNC_DIV_1);
    LL_CRS_SetSyncPolarity(LL_CRS_SYNC_POLARITY_RISING);
    LL_CRS_SetSyncSignalSource(LL_CRS_SYNC_SOURCE_USB);
    LL_CRS_SetReloadCounter(__LL_CRS_CALC_CALCULATE_RELOADVALUE(48000000,1000));
    LL_CRS_SetFreqErrorLimit(LL_CRS_ERRORLIMIT_DEFAULT);
    LL_CRS_SetHSI48SmoothTrimming(LL_CRS_HSI48CALIBRATION_DEFAULT);
    LL_CRS_EnableAutoTrimming();
    LL_CRS_EnableFreqErrorCounter();
#endif
}

void main(void)
{
#if CONFIG_SOC_STM32G474XX
    LL_DBGMCU_EnableDBGSleepMode();
    LL_DBGMCU_EnableDBGStopMode();
    LL_DBGMCU_EnableDBGStandbyMode();
#endif

    enable_crs();

    led_matrix_init();
    usbdev_init();
    led_matrix_loop();
}
