#ifndef APP_LCD_H
#define APP_LCD_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LCD display (SPI Bus + ILI9341 Panel + LVGL)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t app_lcd_init(void);

/**
 * @brief Thread-safe call to display text centered on the screen
 * 
 * @param text The string to render
 */
void app_lcd_set_status_text(const char *text);

#ifdef __cplusplus
}
#endif

#endif // APP_LCD_H