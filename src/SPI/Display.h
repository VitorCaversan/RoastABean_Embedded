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
 * @brief Simple function to change the text of the main label
 * 
 * @param text String to be displayed
 */
extern void display_setUiText(const char* text);

#endif // DISPLAY_H