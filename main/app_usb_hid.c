#include "app_usb_hid.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

#define MAIN_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static const char *TAG = "USB_HID";

// Vendor-Defined HID Report Descriptor (32 bytes)
static const uint8_t custom_hid_report_descriptor[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (Vendor Usage 1)
    0xA1, 0x01,        // Collection (Application)

    // IN Report (Device -> Host)
    0x09, 0x02,        //   Usage (Vendor Usage 2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x40,        //   Report Count (64 bytes)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // OUT Report (Host -> Device)
    0x09, 0x03,        //   Usage (Vendor Usage 3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x40,        //   Report Count (64 bytes)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)

    0xC0               // End Collection
};

// 2. Full Device Descriptor (Mandatory for custom TinyUSB setups)
static const tusb_desc_device_t vendor_device_descriptor = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4004, // Custom PID
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// 3. Complete Configuration Descriptor
static const uint8_t hid_configuration_descriptor[] = {
    // Config Descriptor Header (9 bytes)
    0x09, 0x02, 41, 0x00, 0x01, 0x01, 0x00, 0xC0, 0x32,

    // Interface Descriptor (9 bytes)
    0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,

    // HID Descriptor (9 bytes)
    0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, sizeof(custom_hid_report_descriptor), 0x00,

    // Endpoint IN (7 bytes) - 0x81
    0x07, 0x05, 0x81, 0x03, 0x40, 0x00, 0x01,

    // Endpoint OUT (7 bytes) - 0x01
    0x07, 0x05, 0x01, 0x03, 0x40, 0x00, 0x01
};

// String Descriptors
static const char *string_descriptors[] = {
    (const char[]){0x09, 0x04}, // 0: Language (0x0409 English)
    "Espressif",               // 1: Manufacturer
    "ESP32-S3 HID Device",     // 2: Product
    "123456"                   // 3: Serial
};

// uint8_t const *tud_descriptor_device_cb(void) {
//     return (uint8_t const *)&vendor_device_descriptor;
// }

// uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
//     (void)index;
//     return hid_configuration_descriptor;
// }

uint16_t const *tud_string_desc_sing_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t _desc_str[32];
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_descriptors[0], 2);
        chr_count = 1;
    } else {
        if (index > 3) return NULL;
        const char *str = string_descriptors[index];
        chr_count = strlen(str);
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}

// Required TinyUSB HID Callbacks
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return custom_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type;
    ESP_LOGI(TAG, "Received OUT Report: %d bytes", bufsize);
}

// CRITICAL: Host requires Set Idle to succeed during enumeration
bool tud_hid_set_idle_cb(uint8_t instance, uint8_t idle_rate) {
    (void)instance; (void)idle_rate;
    return true;
}

/*
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t reqlen) {
}
*/

void app_usb_hid_init(void) {

    vendor_hid_init(); // Initialize TinyUSB with Vendor-Defined HID interface
    return;

    ESP_LOGI(TAG, "Initializing USB HID Device...");

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .string_descriptor_count = 0,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB HID Stack initialized successfully.");
}

void app_usb_hid_send_command_id(uint8_t command_id) {
    if (tud_hid_ready()) {
        uint8_t report_buf[64] = {0};
        report_buf[0] = command_id;
        tud_hid_report(HID_REPORT_ID_VOICE_CMD, report_buf, sizeof(report_buf));
        ESP_LOGI(TAG, "Sent Voice Command Byte to USB Host: 0x%02X", command_id);
    } else {
        ESP_LOGW(TAG, "USB HID interface not ready to send.");
    }
}

void vendor_hid_init(void) {
    ESP_LOGI(TAG, "Initializing TinyUSB Stack...");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &vendor_device_descriptor,
        .string_descriptor = string_descriptors,
        .string_descriptor_count = sizeof(string_descriptors) / sizeof(string_descriptors[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    tinyusb_driver_install(&tusb_cfg);
}

void vendor_hid_send_report(const uint8_t *data, uint8_t len) {
    if (!tud_hid_ready()) return;

    tud_hid_report(0, data, len);
    //return tud_hid_report(0, data, len) ? ESP_OK : ESP_FAIL;
}