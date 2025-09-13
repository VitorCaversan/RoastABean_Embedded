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

#define LCD_H_RES_IN_PIX    240
#define LCD_V_RES_IN_PIX    320

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

#endif // DISPLAY_H