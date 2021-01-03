#include <zephyr.h>
#include <drivers/gpio.h>
#include <logging/log.h>

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

void led_matrix_init(void)
{
    LOG_INF("Setting up LED matrix");
    for (int i = 0; i < ARRAY_SIZE(pin_definitions); i++) {
        pins[i].device = device_get_binding(pin_definitions[i].devname);
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

void led_loop(void)
{
    #if !DT_NODE_HAS_STATUS(DT_PATH(matrix, buffer_en), okay)
    while (true) {
        LOG_INF("Spinning");
        k_sleep(K_MSEC(1000));
    }
    return;
    #endif

    while (true) {
        for (int i = 0; i < 64; i++) {
            uint8_t col = 1 << (i % 8);
            uint8_t row = 1 << (i / 8);
            gpio_port_set_masked(pins[1].device, 0xffff, (row << 8) | col);
            k_sleep(K_MSEC(50));
        }
    }
}