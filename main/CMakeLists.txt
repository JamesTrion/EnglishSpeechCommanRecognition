set(srcs
    "main.c"
    "speech_commands_action.c"
    "app_usb_hid.c"
    "app_lcd.c"
    "lcd_text.c"
    "${CMAKE_CURRENT_LIST_DIR}/../managed_components/espressif__tinyusb/src/class/hid/hid_device.c"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "." "include"
    PRIV_REQUIRES espressif__esp_tinyusb espressif__tinyusb hardware_driver esp_lcd lvgl driver
)

# Apply CFG_TUD_HID=1 to the main component library AFTER registration
target_compile_definitions(${COMPONENT_LIB} PUBLIC CFG_TUD_HID=1)