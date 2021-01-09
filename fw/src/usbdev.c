#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(usbdev);

static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
    LOG_INF("USB status %d", status);
}

int set_report(const struct device *dev,
               struct usb_setup_packet *setup, int32_t *len,
               uint8_t **data)
{
    LOG_INF("%s len=%d", __func__, *len);
    LOG_HEXDUMP_INF(*data, *len, "Data");
    return 0;
}

void usbdev_init(void)
{
    const struct device *hid_dev = device_get_binding("HID_0");
    if (!hid_dev) {
        LOG_ERR("Cannot get USB HID Device");
        return;
    }
    LOG_INF("Opened HID device");

    static const struct hid_ops hid_ops = {
        .set_report = set_report,
    };

    usb_hid_register_device(hid_dev,
                            hid_report_desc,
                            sizeof(hid_report_desc),
                            &hid_ops);

    usb_hid_init(hid_dev);

    int ret = usb_enable(status_cb);
    if (ret) {
        LOG_ERR("Failed to enable USB");
        return;
    }

    uint8_t report[4] = { 0x00 };
    while (true) {
        LOG_INF("Moving");
        k_sleep(K_MSEC(10000));
        report[1] = 1;
        ret = hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
        if (ret) {
            LOG_ERR("HID write error, %d", ret);
        }
        k_sleep(K_MSEC(100));
        report[1] = 0;
        ret = hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
        if (ret) {
            LOG_ERR("HID write error, %d", ret);
        }
    }
}