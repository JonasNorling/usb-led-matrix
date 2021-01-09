#include <zephyr.h>
#include <logging/log.h>

#include "led_matrix.h"
#include "usbdev.h"

LOG_MODULE_REGISTER(main);

void main(void)
{
    led_matrix_init();
    usbdev_init();
    led_matrix_loop();
}
