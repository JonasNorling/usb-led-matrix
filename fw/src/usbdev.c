#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(usbdev);

#define LED_COUNT 64

// Vendor defined HID report
static const uint8_t hid_report_desc[] = {
	/* Usage page: vendor defined */
	HID_GLOBAL_ITEM(ITEM_TAG_USAGE_PAGE, 2), 0x00, 0xFF,

	/* Usage: vendor specific */
	HID_LI_USAGE, 0x01,
	/* Collection: Application Collection */
	HID_MI_COLLECTION, COLLECTION_APPLICATION,
	/* Logical Minimum: 0 */
	HID_GI_LOGICAL_MIN(1), 0x00,
	/* Logical Maximum: 255 */
	HID_GI_LOGICAL_MAX(1), 0xFF,
	/* Report Size: 8 bits */
	HID_GI_REPORT_SIZE, 0x08,
	/* Report Count (in bytes) */
	HID_GI_REPORT_COUNT, LED_COUNT,

	/* Report ID: 1 */
	HID_GI_REPORT_ID, 1,
	/* Vendor Usage 2 */
	HID_LI_USAGE, 0x02,
	/* Output: Data, Variable, Absolute & Buffered bytes*/
	HID_MI_OUTPUT, 0x86,

	/* End collection */
	HID_MI_COLLECTION_END,
};

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
}