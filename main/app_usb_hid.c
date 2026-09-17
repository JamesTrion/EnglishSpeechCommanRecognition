#include "device/usbd.h"

#include "app_usb_hid.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "lcd_text.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "device/usbd_pvt.h"

#define MAIN_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static const char *TAG = "USB_HID";

// Vendor-Defined HID Report Descriptor (38 bytes)
static const uint8_t custom_hid_report_descriptor[] = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (Vendor Usage 1)
    0xA1, 0x01,        // Collection (Application)

    // IN Report (Device -> Host)
    0x85, HID_REPORT_ID_VOICE_CMD, // Report ID (1)
    0x09, 0x02,        //   Usage (Vendor Usage 2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x3E,        //   Report Count (62 bytes + 1 byte Report ID = 63 bytes total)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // OUT Report (Host -> Device)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x03,        //   Usage (Vendor Usage 3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x3E,        //   Report Count (62 bytes)
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


void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) report;
    (void) len;
    // Endpoint transmission complete
    // lcd_text_alignment nAlignment=LCD_TEXT_ALIGN_ANY;          //default alignment
    // lcd_text_print_ex(0, 120, 
    //             "Report finished sending", 
    //             RGB565_BLACK, RGB565_WHITE, true, 50, 100, nAlignment); // Clear screen to White and print "Hello World" in Black
}

void app_usb_hid_init(void) 
{
    vendor_hid_init(); // Initialize TinyUSB with Vendor-Defined HID interface
}

void app_usb_hid_send_command_id(uint8_t command_id) 
{

    lcd_text_alignment nAlignment=LCD_TEXT_ALIGN_MIDDLECENTER;          //default alignment

    if (!tud_mounted()) {
        lcd_text_print_ex(0, 120, 
                "USB Host not mounted yet", 
                RGB565_BLACK, RGB565_WHITE, true, 50, 100, nAlignment); // Clear screen to White and print "Hello World" in Black
        return;
    }

    // Wait up to 50ms for the HID interface to clear
    //original code commented out by Trion on 2026/09/16
    int retry = 10;
    while (!tud_hid_ready() && retry > 0) 
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        retry--;
    }
    
    bool bReady=true;
    char display_buf[64];
    memset(display_buf, 0, sizeof(display_buf));
    snprintf(display_buf, sizeof(display_buf), "%s", GetCommandStringFromID(command_id));

    if (tud_hid_ready()==0)
    {
        bReady=false;
        memset(display_buf, 0, sizeof(display_buf));
        snprintf(display_buf, sizeof(display_buf), "%s", GetCommandStringFromID(command_id));
    }

    lcd_text_print_ex(0, 120, display_buf, RGB565_BLACK, RGB565_WHITE, true, 50, 100, nAlignment); // Clear screen to White and print "Hello World" in Black

    uint8_t report_buf[62] = {0}; // Match 62-byte report count
    report_buf[0] = command_id;
        
    bool success = tud_hid_report(HID_REPORT_ID_VOICE_CMD, report_buf, sizeof(report_buf));
    nAlignment=LCD_TEXT_ALIGN_ANY;
    int nAddHeight=60;
    snprintf(display_buf, sizeof(display_buf), "%s runs %d.", GetCommandStringFromID(command_id),success);
    //lcd_text_print_ex(0, 120 + nAddHeight, display_buf, RGB565_BLACK, RGB565_WHITE, false, 50, 100, nAlignment); // Clear screen to White and print "Hello World" in Black

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
