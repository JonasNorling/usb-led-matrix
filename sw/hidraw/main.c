/*
 * User-space driver for the USB-LED-matrix,
 * showing CPU load.
 */

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
#include <dirent.h>

#define MAX_CPUS 32
#define ROWS 8
#define COLS 8
#define SHOW_CPUS ROWS
#define HZ 10
#define PWM_STEPS 8
#define UPDATE_TIME_US (1000000 / HZ)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// USB HID report
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
        "Usage: %s [<hidraw device>]\n",
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

    // Skip first line (cpu)
    pt = strchr(pt, '\n');
    if (pt) pt++;

    // Parse individual cpu lines
    while (pt) {
        int cpu, user, nice, system, idle, iowait;
        res = sscanf(pt, "cpu%d %d %d %d %d %d",
                     &cpu, &user, &nice, &system, &idle, &iowait);
        if (res != 6 || cpu >= MAX_CPUS) {
            // We've gone past the cpu<n> lines
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
    int fd = open(node, O_WRONLY);
    if (fd < 0) {
        return fd;
    }

    char name[256] = "";
    int res = ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name);
    if (res < 0) {
        perror("ioctl failed");
        return res;
    }

    printf("Opened hidraw device %s: %s\n", node, name);

    if (strcmp(name, "Elemental Instruments usb-led-matrix")) {
        close(fd);
        fd = -1;
    }

    return fd;
}

static int64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static void sleep_us(int64_t us)
{
    if (us < 0) {
        return;
    }
    const struct timespec ts = {
        .tv_sec = us / 1000000000,
        .tv_nsec = us * 1000,
    };
    int res = nanosleep(&ts, NULL);
    if (res < 0) {
        perror("Nanosleep failed");
    }
}

static int hid_name_filter(const struct dirent *dirent)
{
    return !strncmp("hidraw", dirent->d_name, 6);
}

static int find_device(void)
{
    int usb_fd = -1;
    struct dirent **namelist = NULL;

    int count = scandir("/dev", &namelist, hid_name_filter, versionsort);
    if (count < 0) {
        perror("Scandir failed");
        return -1;
    }

    for (int i = 0; i < count; i++) {
        char full_name[PATH_MAX];
        snprintf(full_name, PATH_MAX, "/dev/%s", namelist[i]->d_name);
        int fd = open_device(full_name);
        free(namelist[i]);
        if (fd >= 0) {
            usb_fd = fd;
            break;
        }
    }

    free(namelist);
    return usb_fd;
}

static int find_and_open_device(const char *device_name)
{
    int usb_fd = -1;

    if (device_name) {
        usb_fd = open_device(device_name);
        if (usb_fd < 0) {
            perror("Unable to open hidraw device");
            return 1;
        }
    }
    else {
        // Try to find the right device
        for (int i = 0; ; i++) {
            usb_fd = find_device();
            if (usb_fd != -1) {
                break;
            }
            if (i == 0) {
                printf("Waiting for USB device...\n");
            }
            sleep(1);
        }
    }

    return usb_fd;
}

int main(int argc, const char *argv[])
{
    int64_t target_time;
    int usb_fd = -1;
    const char *device_name = NULL;

    if (argc > 2) {
        print_help(argv[0]);
        return 1;
    }
    if (argc > 1) {
        device_name = argv[1];
    }

    usb_fd = find_and_open_device(device_name);
    if (usb_fd < 0) {
        return 1;
    }

    int stat_fd = open("/proc/stat", O_RDONLY);
    if (stat_fd < 0) {
        perror("Error opening /proc/stat");
        return 1;
    }

    /*
     * Put up a static image
     */
    struct report report = { .led_state = {
        0, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 2, 2, 1, 1, 1,
        1, 1, 3, 4, 4, 3, 1, 1,
        1, 2, 4, 8, 8, 4, 2, 1,
        1, 2, 4, 8, 8, 4, 2, 1,
        1, 1, 3, 4, 4, 3, 1, 1,
        1, 1, 1, 2, 2, 1, 1, 1,
        0, 1, 1, 1, 1, 1, 1, 0,
    }};

    int res = write(usb_fd, &report, sizeof(report));
    if (res < 0) {
        fprintf(stderr, "Write returned %d\n", res);
        perror("Failed to write report");
        return 1;
    }
    sleep_us(900000);

    /*
     * Output CPU load
     */
    struct stat_info stat_info = {};
    struct stat_info last_stat_info = {};
    get_stat(stat_fd, &last_stat_info);

    target_time = now_us();
    while (true) {
        get_stat(stat_fd, &stat_info);
        int jiffies[SHOW_CPUS] = {};
        for (int i = 0; i < stat_info.cpus; i++) {
            jiffies[i % SHOW_CPUS] += stat_info.cpu[i].jiffies - last_stat_info.cpu[i].jiffies;
        }
        last_stat_info = stat_info;

        struct report report = {};
        const int jiffies_per_tick = ((stat_info.cpus / SHOW_CPUS) * HZ);
        for (int row = 0; row < ROWS; row++) {
            const float led_count = (float)jiffies[row] * COLS / jiffies_per_tick;
            for (int col = 0; col < COLS; col++) {
                const float brightness = PWM_STEPS * (led_count - col);
                report.led_state[(row * 8) + col] = MAX(MIN(brightness, 8), 0);
            }
        }

        int res = write(usb_fd, &report, sizeof(report));
        if (res < 0) {
            fprintf(stderr, "Write returned %d\n", res);
            perror("Failed to write report");
            return 1;
        }

        target_time += UPDATE_TIME_US;
        sleep_us(target_time - now_us());
    }

    close(usb_fd);
    close(stat_fd);
    return 0;
}
