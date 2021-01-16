#include <zephyr.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <irq.h>
#include <soc.h>

#include "led_matrix.h"

LOG_MODULE_REGISTER(led_matrix);

#define TIMER TIM1
#if CONFIG_SOC_SERIES_STM32G4X
#define IRQ_NUMBER TIM1_UP_TIM16_IRQn
#elif CONFIG_SOC_SERIES_STM32F4X
#define IRQ_NUMBER TIM1_UP_TIM10_IRQn
#endif

#define GPIO_PIN(dt_node, flags) { DT_GPIO_LABEL(dt_node, gpios), \
                                   DT_GPIO_PIN(dt_node, gpios), \
                                   (flags) | DT_GPIO_FLAGS(dt_node, gpios) }

struct pin_definition {
    const char *devname;
    gpio_pin_t pin;
    gpio_flags_t flags;
};

struct pin {
    const struct device *device;
    gpio_pin_t pin;
};

static const struct pin_definition pin_definitions[] = {
#if DT_NODE_HAS_STATUS(DT_PATH(matrix, buffer_en), okay)
    GPIO_PIN(DT_PATH(matrix, buffer_en), GPIO_OUTPUT_ACTIVE),
    GPIO_PIN(DT_PATH(matrix, row1), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row2), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row3), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row4), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row5), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row6), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row7), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, row8), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col1), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col2), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col3), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col4), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col5), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col6), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col7), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(matrix, col8), GPIO_OUTPUT_INACTIVE),
#elif DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
    GPIO_PIN(DT_ALIAS(led0), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_ALIAS(led1), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_ALIAS(led2), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_ALIAS(led3), GPIO_OUTPUT_INACTIVE),
#else
#warning Building without LED matrix support
#endif
};

static struct pin pins[ARRAY_SIZE(pin_definitions)];
static volatile uint8_t led_data[LED_COUNT];


static bool get_led_state(int col, int row)
{
    return led_data[(row * 8) + (col % 8)];
}

static void timer_isr(const void *arg)
{
    LL_TIM_ClearFlag_UPDATE(TIMER);

//#if DT_NODE_HAS_STATUS(DT_PATH(matrix, buffer_en), okay)
    static uint8_t colno = 0;
    colno++;
    uint8_t col = 1 << (colno % 8);
    uint8_t row = 0;
    //gpio_port_set_masked(pins[1].device, 0xffff, 0);
    for (int rowno = 0; rowno < 8; rowno++) {
        row |= get_led_state(colno, rowno) ? (1 << rowno) : 0;
    }
    gpio_port_set_masked(pins[1].device, 0xffff, (row << 8) | col);
//#endif
}

void led_matrix_init(void)
{
    LOG_INF("Setting up LED matrix");
    for (int i = 0; i < ARRAY_SIZE(pin_definitions); i++) {
        pins[i].device = device_get_binding(pin_definitions[i].devname);
        pins[i].pin = pin_definitions[i].pin;
        if (!pins[i].device) {
            LOG_ERR("No device");
            return;
        }
        int rc = gpio_pin_configure(pins[i].device,
                                    pin_definitions[i].pin,
                                    pin_definitions[i].flags);
        if (rc) {
            LOG_ERR("Configure failed");
            return;
        }
    }

    led_data[0] = 1;

#if CONFIG_SOC_FAMILY_STM32
    IRQ_CONNECT(IRQ_NUMBER, 2, timer_isr, NULL, 0);
    irq_enable(IRQ_NUMBER);

    __HAL_RCC_TIM1_CLK_ENABLE();
    LL_TIM_InitTypeDef init;
    LL_TIM_StructInit(&init);
    //init.Prescaler = 2594;  // Makes 1 Hz interrupts
    init.Prescaler = 1;
    LL_TIM_Init(TIMER, &init);
    LL_TIM_EnableIT_UPDATE(TIMER);
#endif
}

void led_matrix_start(void)
{
#if CONFIG_SOC_FAMILY_STM32
    LL_TIM_EnableCounter(TIMER);
#endif
}

void led_matrix_set(const uint8_t *data, size_t len)
{
    memcpy(led_data, data, MIN(len, sizeof(led_data)));
}
