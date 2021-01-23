#include <zephyr.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "led_matrix.h"

LOG_MODULE_REGISTER(led_matrix);

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
#else
#warning Building without LED matrix support
#endif
};

static struct pin pins[ARRAY_SIZE(pin_definitions)];

static uint8_t led_data[LED_COUNT];


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
}

static bool get_led_state(int col, int row)
{
    return led_data[(row * 8) + (col % 8)];
}

void led_matrix_loop(void)
{
    #if !DT_NODE_HAS_STATUS(DT_PATH(matrix, buffer_en), okay)
    while (true) {
        LOG_INF("Spinning");
        k_sleep(K_MSEC(1000));
    }
    return;
    #endif

    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t col = 1 << (i % 8);
        uint8_t row = 1 << (i / 8);
        gpio_port_set_masked(pins[1].device, 0xffff, (row << 8) | col);
        k_sleep(K_MSEC(50));
    }

    while (true) {
        for (int colno = 0; colno < 8; colno++) {
            const uint8_t col = 1 << colno;
            uint8_t row = 0;
            for (int rowno = 0; rowno < 8; rowno++) {
                row |= get_led_state(col, row) ? 1 << rowno : 0;
            }
            gpio_port_set_masked(pins[1].device, 0xffff, (row << 8) | col);
            k_sleep(K_MSEC(10));
            gpio_port_set_masked(pins[1].device, 0xffff, 0);
        }
    }
}

void led_matrix_set(const uint8_t *data, size_t len)
{
    memcpy(led_data, data, MIN(len, sizeof(led_data)));
}
