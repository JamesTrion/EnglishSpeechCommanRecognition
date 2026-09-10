#ifndef APP_USB_HID_H
#define APP_USB_HID_H

#include <stdint.h>

#define HID_REPORT_ID_VOICE_CMD 0x01

void app_usb_hid_init(void);
void app_usb_hid_send_command_id(uint8_t command_id);

#endif