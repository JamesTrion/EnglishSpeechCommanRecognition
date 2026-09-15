#ifndef LCD_TEXT_H
#define LCD_TEXT_H

#include <stdint.h>
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_HOST               SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define LCD_H_RES              320
#define LCD_V_RES              240

#define PIN_NUM_SCLK           7
#define PIN_NUM_MOSI           6
#define PIN_NUM_MISO           -1
#define PIN_NUM_LCD_DC         4
#define PIN_NUM_LCD_CS         5
#define PIN_NUM_LCD_RST        48
#define PIN_NUM_BK_LIGHT       47   //45

#define PIN_NUM_CLK 7
#define PIN_NUM_DC  4
#define PIN_NUM_CS  5
#define PIN_NUM_RST 48


//added by Trion on 2026/09/10
#define RGB565_BLACK    0x0000
#define RGB565_WHITE    0xFFFF
#define RGB565_RED      0xF800
#define RGB565_GREEN    0x07E0
#define RGB565_BLUE     0x001F
#define RGB565_YELLOW   0xFFE0
#define RGB565_CYAN     0x07FF
#define RGB565_MAGENTA  0xF81F
#define RGB565_ORANGE   0xFC00
//added by Trion on 2026/09/10

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16


typedef enum 
{
    Command_MoveLeft = 0,
    Command_MoveRight,
    Command_PageUp,
    Command_PageDown,
    Command_ZoomIn,
    Command_ZoomOut,
    Command_DragUp,
    Command_DragDown,
    Command_DragLeft,
    Command_DragRight,
    Command_RotateLeft,
    Command_RotateRight,

    Command_ListeningForWakeword,
    Command_WakewordDetected,
    Command_Timeout

} command_message_id;


static const char* SpeechCommandsInString[] = 
{
    "Move Left",
    "Move Right",
    "Page Up",
    "Page Down",
    "Zoom In",
    "Zoom Out",
    "Drag Up",
    "Drag Down",
    "Drag Left",
    "Drag Right",
    "Rotate Left",
    "Rotate Right",

    "Listening for Wakeword...",
    "Wakeword Detected! Waiting Command...",
    "Timeout! Awaiting Wakeword..."
};

//static const int SpeechCommandsCount = sizeof(SpeechCommandsInString) / sizeof(SpeechCommandsInString[0]);  



typedef enum 
{
    LCD_TEXT_ALIGN_ANY,
    
    LCD_TEXT_ALIGN_TOPLEFT,
    LCD_TEXT_ALIGN_TOPCENTER,
    LCD_TEXT_ALIGN_TOPRIGHT,

    LCD_TEXT_ALIGN_MIDDLELEFT,
    LCD_TEXT_ALIGN_MIDDLECENTER,    
    LCD_TEXT_ALIGN_MIDDLELRIGHT,

    LCD_TEXT_ALIGN_BOTTOMLEFT,
    LCD_TEXT_ALIGN_BOTTOMCENTER,    
    LCD_TEXT_ALIGN_BOTTOMRIGHT,
    
} lcd_text_alignment;


//void lcd_init(void);

/**
 * @brief Initialize the SPI display controller and backlight.
 * @return ESP_OK on success.
 */
esp_err_t lcd_text_init(void);

/**
 * @brief Clear the entire LCD screen with a single RGB565 color.
 * @param color RGB565 color code (e.g., 0x0000 for Black).
 */
void lcd_text_clear(uint16_t color);

/**
 * @brief Output a line of string at specific (x, y) pixel coordinates.
 * 
 * @param x Start X coordinate (0 - 319)
 * @param y Start Y coordinate (0 - 239)
 * @param text Null-terminated string to render
 * @param fg_color RGB565 text color (e.g., 0xFFFF for White)
 * @param bg_color RGB565 background color (e.g., 0x0000 for Black)
 */
void lcd_text_print(uint16_t x, uint16_t y, const char *text, uint16_t fg_color, uint16_t bg_color);

//convert to rgb565 from 24bit html color code.
uint16_t html_to_rgb565(const char *html_hex);

//added by Trion on 2026/09/10
//combined function to print text with optional screen clear and wait times
void lcd_text_print_ex(uint16_t x, uint16_t y, const char *text, uint16_t fg_color, uint16_t bg_color, 
    bool bClearScreen,uint16_t unClearScreenWaitTime,uint16_t unWaitTimeAfterPrint, lcd_text_alignment alignment);

const char * GetCommandStringFromID(command_message_id command_id);
int GetSpeechCommandsCount(void);

#ifdef __cplusplus
}
#endif

#endif // LCD_TEXT_H