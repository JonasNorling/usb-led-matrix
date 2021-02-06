#include <zephyr.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <irq.h>
#include <soc.h>
#include <stdio.h>

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

#define PIN_TP4 17

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
    [PIN_TP4] = GPIO_PIN(DT_PATH(testpoints, tp4), GPIO_OUTPUT_INACTIVE),
    GPIO_PIN(DT_PATH(testpoints, tp5), GPIO_INPUT),
    GPIO_PIN(DT_PATH(testpoints, tp6), GPIO_INPUT),
    GPIO_PIN(DT_PATH(testpoints, tp7), GPIO_INPUT),
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
static uint8_t led_data[LED_COUNT];
static bool usb_active = false;

#if CONFIG_SOC_FAMILY_STM32
static uint8_t get_led_state(int col, int row)
{
    return led_data[(row * 8) + (col % 8)];
}

static void timer_isr(const void *arg)
{
    gpio_pin_set(pins[PIN_TP4].device, pins[PIN_TP4].pin, true);
    LL_TIM_ClearFlag_UPDATE(TIMER);

    static uint8_t counter = 0;
    counter++;

    const uint8_t colno = counter % 8;
    const uint8_t pwm = (counter / 8) % 16;

    uint8_t row = 0;
    for (int rowno = 0; rowno < 8; rowno++) {
        const uint8_t intensity = get_led_state(colno, rowno);
        const bool on = intensity > pwm;
        row |= on ? (1 << rowno) : 0;
    }

    const uint8_t colbits = 1 << colno;
    gpio_port_set_masked(pins[1].device, 0xffff, (row << 8) | colbits);
    gpio_pin_set(pins[PIN_TP4].device, pins[PIN_TP4].pin, false);
}
#endif

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
    init.Prescaler = 1;
    init.Autoreload = 256;
    LL_TIM_Init(TIMER, &init);
    LL_TIM_EnableIT_UPDATE(TIMER);
#endif
}

static void test_patterns()
{
    uint8_t data[LED_COUNT];

    // All on
    memset(data, 0xff, sizeof(data));
    led_matrix_set(data, LED_COUNT);
    k_sleep(K_MSEC(500));

    // Fade out
    for (int i = 8; i >= 0; i--) {
        memset(data, i, sizeof(data));
        led_matrix_set(data, LED_COUNT);
        k_sleep(K_MSEC(100));
    }

    // Rows
    for (int row = 0; row < 8; row++) {
        memset(data, 0x00, sizeof(data));
        for (int col = 0; col < 8; col++) {
            data[(row * 8) + (col % 8)] = 1;
        }
        led_matrix_set(data, LED_COUNT);
        k_sleep(K_MSEC(125));
    }

    // Columns
    for (int col = 0; col < 8; col++) {
        memset(data, 0x00, sizeof(data));
        for (int row = 0; row < 8; row++) {
            data[(row * 8) + (col % 8)] = 1;
        }
        led_matrix_set(data, LED_COUNT);
        k_sleep(K_MSEC(125));
    }

    // A cross
    for (int col = 0; col < 8; col++) {
        for (int row = 0; row < 8; row++) {
            data[(row * 8) + (col % 8)] = ((row - col) == 0) || ((row + col) == 7) ;
        }
    }
    led_matrix_set(data, LED_COUNT);
}

#define KSZ 5
static float get(float frame[8][8], int x, int y)
{
    if (x < 0) return 0.0;
    if (y < 0) return 0.0;
    if (x >= 8) return 0.0;
    if (y >= 8) return 0.0;
    return frame[x][y];
}

static void step_animation(void)
{
    static float kernel[KSZ][KSZ] = {
        {    0,    0,    0,     0,    0 },
        {    0,    0,  0.1, -0.08,    0 },
        {    0,  0.1,  0.8,   0.1,    0 },
        {    0,    0,  0.1, -0.08,    0 },
        {    0,    0,    0,     0,    0 },
    };
    static float frame[8][8] = {{ 0 },
                                { 0 },
                                { 0 },
                                { 0, 0, 0, 0, 20.0 }};
    float next_frame[8][8];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            float v = 0;
            for (int x = 0; x < KSZ; x++) {
                for (int y = 0; y < KSZ; y++) {
                    v += get(frame, col + x - KSZ/2, row + y - KSZ/2) * kernel[x][y];
                }
            }
            next_frame[col][row] = v;
        }
    }

    uint8_t buffer[LED_COUNT];
    float total = 0;
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            buffer[(row * 8) + (col % 8)] = next_frame[col][row];
            frame[col][row] = next_frame[col][row];
            total += (next_frame[col][row] > 0) ? next_frame[col][row] : 0;
        }
    }
    led_matrix_set(buffer, sizeof(buffer));
    if (total < 2.0f) {
        frame[4][4] = 50.0;
    }
#if CONFIG_BOARD_NATIVE_POSIX
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            const char c[9] = { ' ', '1', '2', '3', '4', '5', '6', '7', '@' };
            int v = frame[row][col];
            if (v < 0) v = 0;
            if (v > 8) v = 8;
            printf("%c ", c[v]);
        }
        printf("\n");
    }
    printf("----------------\n");
#endif
}

void led_matrix_start(void)
{
#if CONFIG_SOC_FAMILY_STM32
    LL_TIM_EnableCounter(TIMER);
#endif

    test_patterns();
    while (true) {
        if (!usb_active) {
            step_animation();
        }
        k_sleep(K_MSEC(20));
    }
}

void led_matrix_set(const uint8_t *data, size_t len)
{
    memcpy(led_data, data, MIN(len, sizeof(led_data)));
    //usb_active = true;
}
