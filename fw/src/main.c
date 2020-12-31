#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(main);

int main(int argc, const char *argv[])
{
    LOG_INF("Starting thing");
    return 0;
}
