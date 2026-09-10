#include "app_usb_hid.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"

static const char *TAG = "USB_HID";

// Custom Vendor-Defined Report Descriptor (1 Byte Input Report)
/*  //marked by Trion on 2026/09/03
static const uint8_t custom_hid_report_descriptor[] = {
    TUSB_DESC_HID_REPORT_DESC_GENERIC_IN(1, HID_REPORT_ID_VOICE_CMD)
};
*/

//added amd modified by Trion on 2026/09/03
static const uint8_t custom_hid_report_descriptor[] = {
    // Standard Generic HID Report Descriptor
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (Vendor Usage 1)
    0xA1, 0x01,        // Collection (Application)
    
    // IN Report
    0x85, HID_REPORT_ID_VOICE_CMD, // Report ID
    0x75, 0x08,                    // Report Size: 8 bits
    0x95, 0x40,                    // Report Count: 64 bytes
    0x09, 0x02,                    // Usage (Vendor Usage 2)
    0x15, 0x00,                    // Logical Minimum (0)
    0x26, 0xFF, 0x00,              // Logical Maximum (255)
    0x81, 0x02,                    // Input (Data, Variable, Absolute)
    
    0xC0               // End Collection
};

// Configuration Descriptor
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration descriptor header
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0, 0),
    // HID Interface descriptor
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(custom_hid_report_descriptor), 0x81, CFG_TUD_HID_EP_BUFSIZE, 10)
};

// String Descriptors
static const char *string_descriptors[] = {
    (char[]){0x09, 0x04}, // 0: English
    "Espressif",          // 1: Manufacturer
    "ESP32-S3 Voice HID", // 2: Product
    "123456"              // 3: Serial Number
};

// TinyUSB Callbacks
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    return custom_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t reqlen) {
}

void app_usb_hid_init(void) {
    ESP_LOGI(TAG, "Initializing Custom USB HID Device...");
    
    tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = string_descriptors,
        .string_descriptor_count = sizeof(string_descriptors) / sizeof(string_descriptors[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB HID Stack initialized successfully.");
}

/*  //original codes, marked by Trion on 2026/09/03
void app_usb_hid_send_command_id(uint8_t command_id) {
    if (tud_hid_ready()) {
        // Send byte payload matching Report ID 0x01
        tud_hid_report(HID_REPORT_ID_VOICE_CMD, &command_id, sizeof(command_id));
        ESP_LOGI(TAG, "Sent Voice Command Byte to USB Host: 0x%02X", command_id);
    } else {
        ESP_LOGW(TAG, "USB HID interface not ready to send.");
    }
}
*/

void app_usb_hid_send_command_id(uint8_t command_id) 
{
    if (tud_hid_ready()) 
    {
        uint8_t report_buf[64] = {0};
        report_buf[0] = command_id;
        tud_hid_report(HID_REPORT_ID_VOICE_CMD, report_buf, sizeof(report_buf));
        ESP_LOGI(TAG, "Sent Voice Command Byte to USB Host: 0x%02X", command_id);
    } 
    else 
    {
        ESP_LOGW(TAG, "USB HID interface not ready to send.");
    }
}