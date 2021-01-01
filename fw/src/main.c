#include <zephyr.h>
#include <logging/log.h>

#include "led_matrix.h"

LOG_MODULE_REGISTER(main);

int main(int argc, const char *argv[])
{
    LOG_INF("Starting thing");

    led_matrix_init();
    led_loop();

    return 0;
}
