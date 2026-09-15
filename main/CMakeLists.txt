idf_component_register(
    SRCS 
        "main.c"
        "speech_commands_action.c"
        "app_usb_hid.c"
        "app_lcd.c"
        "lcd_text.c"
    INCLUDE_DIRS "." "include"
    PRIV_REQUIRES espressif__esp_tinyusb espressif__tinyusb hardware_driver esp_lcd lvgl driver
)

# Force CFG_TUD_HID=1 on both the main library AND tinyusb component
target_compile_definitions(${COMPONENT_LIB} PUBLIC -DCFG_TUD_HID=1)
if(TARGET espressif__tinyusb)
    target_compile_definitions(espressif__tinyusb PUBLIC -DCFG_TUD_HID=1)
endif()