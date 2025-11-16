#ifndef DISPLAY_H
#define DISPLAY_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
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
#include "Buttons.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define USE_LCD_ILI9341  1  // Set to 0 to use ST7789

#define LCD_H_RES_IN_PIX    320
#if USE_LCD_ILI9341
#define LCD_V_RES_IN_PIX    240
#else
#define LCD_V_RES_IN_PIX    240
#endif

#define MAX_ROAST_TIME_IN_MIN       200

#define MAX_VERTICAL_MENU_OPTION_COUNT    10

#define MAX_CHART_NAMESIZE    64

#define MAIN_MENU_OPTION_COUNT      3
#define SELECT_ROAST_MENU_OPTION_COUNT    3

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef struct ST_RoastChartUi
{
    lv_obj_t *screen;
    lv_obj_t *chart;
    lv_chart_series_t *seriesTarget;
    lv_chart_series_t *seriesCurrent;
    lv_obj_t *labelInfo;
    uint32_t totalMins;
    float yMin;
    float yMax;

    lv_obj_t *bottomBar;
    lv_obj_t *btnReturn;

    lv_style_t styleBottomBar;
} ST_RoastChartUi;

typedef struct ST_chartUpdateData
{
    float currTargetTemp;
    uint32_t elapsedSecs;
    float currentTemp;
} ST_chartUpdateData;

typedef struct ST_VerticalMenuUi
{
    lv_obj_t *screen;
    lv_obj_t *menuContainer;
    lv_obj_t *menuOptions[MAX_VERTICAL_MENU_OPTION_COUNT];
    lv_obj_t *bottomBar;
    lv_obj_t *btnReturn;
    lv_obj_t *btnUp;
    lv_obj_t *btnDown;
    lv_obj_t *btnSelect;

    lv_style_t styleItem;
    lv_style_t styleItemSelected;
    lv_style_t styleBottomBar;

    int optionQty;
    int selectedIndex;
} ST_VerticalMenuUi;

typedef struct ST_PopupUi
{
    lv_obj_t *overlay;         // Semi-transparent background
    lv_obj_t *container;       // White balloon container
    lv_obj_t *messageLabel;    // Message text
    lv_obj_t *bottomBar;       // Bottom button bar overlay
    lv_obj_t *btnCancel;       // Left button (cancel)
    lv_obj_t *btnConfirm;      // Right button (confirm)

    lv_style_t styleContainer;
    lv_style_t styleBottomBar;
} ST_PopupUi;

typedef struct ST_storedChart
{
    char chartName[MAX_CHART_NAMESIZE];
    float tempProfile[MAX_ROAST_TIME_IN_MIN];
    uint32_t totalMins;
} ST_storedChart;

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

/**
 * @brief Create a Roast Chart UI screen
 * 
 * @param profile The temperature profile array (°C/min)
 * @param totalMins Total duration of the roast (minutes)
 * @param yMin Minimum Y-axis value. If both NAN, auto-fit from profile
 * @param yMax Maximum Y-axis value. If both NAN, auto-fit from profile
 */
extern void display_createRoastChart(const float *profile, uint32_t totalMins, float yMin, float yMax);

/**
 * @brief Show the Roast Chart screen
 */
extern void display_showRoastChart(void);

/**
 * @brief Update the current temperature point at the corresponding second
 * and refresh the info label.
 *
 * @param currTargetTemp Current target temperature at this moment (°C)
 * @param elapsedSecs seconds since roast start
 * @param currentTemp latest measured temperature (°C)
 */
extern void display_updateRoastChart(float currTargetTemp, uint32_t elapsedSecs, float currentTemp);

/**
 * @brief Create the start menu UI
 */
extern void display_createMainMenu();

/**
 * @brief Show the start menu screen
 */
extern void display_showMainMenu(void);

/**
 * @brief Apply the selection style to the start menu UI
 * 
 * @param ui Pointer to the start menu UI structure
 * @param optionQty Number of menu options available
 */
extern void display_applySelectionStyle(ST_VerticalMenuUi *ui, int optionQty);

/**
 * @brief Get pointer to the start menu UI structure
 * 
 * @return Pointer to the start menu UI structure
 */
extern ST_VerticalMenuUi *display_getMainMenuUi(void);

/**
 * @brief Create the select roast menu UI
 */
extern void display_createSelectRoastMenu(void);

/**
 * @brief Show the select roast menu screen
 */
extern void display_showSelectRoastMenu(ST_storedChart *charts, uint8_t chartCount);

/**
 * @brief Get pointer to the select roast menu UI structure
 * 
 * @return Pointer to the select roast menu UI structure
 */
extern ST_VerticalMenuUi *display_getSelectRoastMenuUi(void);

/**
 * @brief Create a confirmation popup with message
 * 
 * @param message The message to display in the popup. \0 terminated string.
 */
extern void display_createConfirmationPopup(const char *message, lv_color_t currScrBckgnd);

/**
 * @brief Show the confirmation popup overlay
 */
extern void display_showConfirmationPopup(void);

/**
 * @brief Hide the confirmation popup overlay
 */
extern void display_hideConfirmationPopup(void);

#endif // DISPLAY_H