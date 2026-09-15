#ifndef APP_USB_HID_H
#define APP_USB_HID_H

#include <stdint.h>

#define HID_REPORT_ID_VOICE_CMD 0x01
#define VENDOR_HID_REPORT_SIZE 64

void app_usb_hid_init(void);
void app_usb_hid_send_command_id(uint8_t command_id);

/**
 * @brief Initialize TinyUSB with Vendor-Defined HID interface
 */
void vendor_hid_init(void);

/**
 * @brief Send a 64-byte raw HID report to the host
 */
void vendor_hid_send_report(const uint8_t *data, uint8_t len);

#endif