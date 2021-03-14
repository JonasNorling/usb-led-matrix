#include <zephyr.h>
#include <logging/log.h>

#include "led_matrix.h"
#include "usbdev.h"

#if CONFIG_SOC_STM32G474XX
#include <stm32g4xx_ll_crs.h>
#endif

LOG_MODULE_REGISTER(main);

/*
 * Enable clock recovery to make USB work with internal oscillator
 */
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

/*
 * Configure brown-out reset at a reasonable voltage level
 */
static void configure_bor()
{
#if CONFIG_SOC_STM32G474XX
    FLASH_OBProgramInitTypeDef ob = {};
    HAL_FLASHEx_OBGetConfig(&ob);

    if ((ob.USERConfig & FLASH_OPTR_BOR_LEV_Msk) != FLASH_OPTR_BOR_LEV_3) {
        LOG_INF("User option bytes: %08x", ob.USERConfig);
        static const FLASH_OBProgramInitTypeDef ob_write = {
            .OptionType = OPTIONBYTE_USER,
            .USERType = OB_USER_BOR_LEV,
            .USERConfig = OB_BOR_LEVEL_3,
        };
        if (HAL_FLASH_Unlock() != HAL_OK) {
            LOG_ERR("Failed to unlock flash");
        }
        if (HAL_FLASH_OB_Unlock() != HAL_OK) {
            LOG_ERR("Failed to unlock OB");
        }
        if (HAL_FLASHEx_OBProgram((FLASH_OBProgramInitTypeDef*)&ob_write) != HAL_OK) {
            LOG_ERR("Failed to program option bytes");
        }
        if (HAL_FLASH_OB_Lock() != HAL_OK) {
            LOG_ERR("Failed to lock OB");
        }
        if (HAL_FLASH_Lock() != HAL_OK) {
            LOG_ERR("Failed to lock flash");
        }
    }
#endif
}

void main(void)
{
#if CONFIG_SOC_STM32G474XX
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStopMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
#endif

    led_matrix_init();
    enable_crs();
    configure_bor();
    usbdev_init();
    led_matrix_start();

    while (true) {
        k_sleep(K_FOREVER);
    }
}
