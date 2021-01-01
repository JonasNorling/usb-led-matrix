#include <zephyr.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include "led_matrix.h"

LOG_MODULE_REGISTER(main);

static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
    LOG_INF("USB status %d", status);
}

int main(int argc, const char *argv[])
{
    LOG_INF("Starting thing");

    led_matrix_init();

	const struct device *hid_dev = device_get_binding("HID_0");
	if (!hid_dev) {
		LOG_ERR("Cannot get USB HID Device");
		k_panic();
	}

	usb_hid_register_device(hid_dev,
				            hid_report_desc,
                            sizeof(hid_report_desc),
				            NULL);

	usb_hid_init(hid_dev);

	int ret = usb_enable(status_cb);
	if (ret) {
		LOG_ERR("Failed to enable USB");
		k_panic();
	}

    led_loop();

    return 0;
}
