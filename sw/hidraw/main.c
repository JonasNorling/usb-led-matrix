#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_CPUS 32

__attribute__((packed)) struct report {
    uint8_t report_no;
    uint8_t led_state[64];
};

struct stat_info {
    int cpus;
    struct {
        int jiffies;
    } cpu[MAX_CPUS];
};


static void print_help(const char *progname)
{
    printf(
        "Usage: %s <hidraw device>\n",
        progname);
}

static void get_stat(int fd, struct stat_info *stat_info)
{
    // This function is a segmentation fault waiting to happen

    char buf[4096] = "";
    int res = pread(fd, buf, sizeof(buf), 0);
    if (res < 0) {
        perror("Failed to read /proc/stat");
        return;
    }

    const char *pt = buf;
    pt = strchr(pt, '\n');
    if (pt) pt++;
    while (pt) {
        int cpu, user, nice, system, idle, iowait;
        res = sscanf(pt, "cpu%d %d %d %d %d %d", &cpu, &user, &nice, &system, &idle, &iowait);
        if (res != 6) {
            break;
        }
        stat_info->cpu[cpu].jiffies = user + nice + system + iowait;
        stat_info->cpus = cpu + 1;
        pt = strchr(pt, '\n');
        if (pt) pt++;
    }
}

static int open_device(const char *node)
{
    int fd = open("/dev/hidraw1", O_WRONLY);
    if (fd < 0) {
        perror("Unable to open hidraw device");
        return fd;
    }

    char name[256] = "";
    int res = ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name);
    if (res < 0) {
        perror("ioctl failed");
        return res;
    }
    
    printf("Opened hidraw device for %s\n", name);

    return fd;
}

int main(int argc, const char *argv[])
{
    if (argc != 2) {
        print_help(argv[0]);
        return 1;
    }
    const char *device_name = argv[1];

    int usb_fd = open_device(device_name);
    if (usb_fd < 0) {
        return 1;
    }

    int stat_fd = open("/proc/stat", O_RDONLY);
    if (stat_fd < 0) {
        perror("Error opening /proc/stat");
        return 1;
    }

    for (int i = 0; i < 64; i++) {
        struct report report = {};
        report.led_state[i] = 1;

        int res = write(usb_fd, &report, sizeof(report));
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

    struct stat_info stat_info = {};
    struct stat_info last_stat_info = {};
    get_stat(stat_fd, &last_stat_info);

    while (true) {
        get_stat(stat_fd, &stat_info);
        int jiffies[8] = {};
        for (int i = 0; i < stat_info.cpus; i++) {
            jiffies[i % 8] += stat_info.cpu[i].jiffies - last_stat_info.cpu[i].jiffies;
        }
        last_stat_info = stat_info;

        struct report report = {};
        for (int row = 0; row < 8; row++) {
            int leds = jiffies[row] * 8 / 200;
            for (int col = 0; col < 8; col++) {
                report.led_state[(row * 8) + col] = col < leds ? 1 : 0;
            }
        }

        int res = write(usb_fd, &report, sizeof(report));
        if (res < 0) {
            fprintf(stderr, "Write returned %d\n", res);
            perror("Failed to write report");
        }
    }

    close(usb_fd);
    close(stat_fd);
    return 0;
}