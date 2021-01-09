#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

__attribute__((packed)) struct report {
    uint8_t report_no;
    uint8_t led_state[64];
};

int main(int argc, const char *argv[])
{
    printf("Hej\n");

    int fd = open("/dev/hidraw1", O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        perror("Unable to open hidraw device");
        return 1;
    }

    for (int i = 0; i < 64; i++) {
        struct report report = {};
        report.led_state[i] = 1;

        int res = write(fd, &report, sizeof(report));
        if (res < 0) {
            fprintf(stderr, "Write returned %d\n", res);
            perror("Failed to write report");
        }
/*
        const struct timespec ts = { .tv_nsec = 500000000 };
        res = nanosleep(&ts, NULL);
        if (res < 0) {
            perror("Nanosleep failed");
        }
*/
    }

    close(fd);
    return 0;
}