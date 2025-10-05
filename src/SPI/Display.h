#ifndef DISPLAY_H
#define DISPLAY_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "driver/ledc.h"

#include "SpiConfig.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define USE_LCD_ILI9341  0  // Set to 0 to use ST7789

#define LCD_H_RES_IN_PIX    240
#if USE_LCD_ILI9341
#define LCD_V_RES_IN_PIX    320
#else
#define LCD_V_RES_IN_PIX    240
#endif

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initialize the LCD display peripheral and backlight
 */
extern void display_lcdInit(void);

/**
 * @brief Initialize LVGL library and create a simple UI
 */
extern void display_uiInit(void);

/**
 * @brief Set the display brightness
 * 
 * @param brightness Value from 0.0 to 100.0
 */
extern void display_setDispBrightness(float brightness);

/**
 * @brief Simple function to change the text of the main label
 * 
 * @param text String to be displayed
 */
extern void display_setUiText(const char* text);

/**
 * @brief Self contained function to create and update a status bar at the top of the screen
 * 
 * @param ethUp If ethernet is up (true) or down (false)
 * @param tempC Temperature in celsius
 * @param pwm PWM value (0-100)
 * @param fault If system is in fault state (true) or normal (false)
 * @param timeStr Formatted time string (e.g., "12:34"), or NULL/empty for no time
 */
extern void display_uiStatusBarUpdate(bool ethUp, float tempC, int pwm, bool fault, const char *timeStr);

#endif // DISPLAY_H