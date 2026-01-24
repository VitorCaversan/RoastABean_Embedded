/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Display.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define PIN_CS       GPIO_NUM_16
#define PIN_DC       GPIO_NUM_18
#define PIN_RST      46 // -1 Not used
#define PIN_LED      GPIO_NUM_3
#define LCD_MAX_CLK_HZ        10000000  // 10MHz

#define LCD_BCKLIGHT_MODE            LEDC_LOW_SPEED_MODE   // low-speed works on all pins on S3
#define LCD_BCKLIGHT_TIMER           LEDC_TIMER_0
#define LCD_BCKLIGHT_CHANNEL         LEDC_CHANNEL_0
#define LCD_BCKLIGHT_DUTY_RES        LEDC_TIMER_10_BIT

#define INITIAL_BCKLIGHT_DUTY_PERCENT   80.0f

// UI Layout defines
#define UI_TITLE_Y_OFFSET               6
#define UI_TITLE_Y_OFFSET_SMALL         8
#define UI_CHART_CENTER_Y_OFFSET        20
#define UI_CHART_WIDTH_PCT              85
#define UI_CHART_HEIGHT_PCT             60
#define UI_CHART_DIV_LINES              6
#define UI_CHART_TITLE_X_OFFSET         20
#define UI_CHART_AXIS_LABEL_X_OFFSET    -4
#define UI_CHART_AXIS_LABEL_Y_OFFSET    2
#define UI_CHART_LEGEND_Y_OFFSET        -4
#define UI_CHART_INFO_Y_OFFSET          20

#define UI_POPUP_CONTAINER_WIDTH_PCT    80
#define UI_POPUP_CONTAINER_PADDING      16
#define UI_POPUP_CONTAINER_BORDER       2
#define UI_POPUP_CONTAINER_RADIUS       12
#define UI_POPUP_SHADOW_WIDTH           20

#define UI_MENU_CONTAINER_WIDTH_PCT     98
#define UI_MENU_CONTAINER_HEIGHT_PCT    70
#define UI_MENU_CONTAINER_Y_OFFSET      26
#define UI_MENU_OPTION_WIDTH_PCT        94
#define UI_MENU_OPTION_HEIGHT           36
#define UI_MENU_OPTION_RADIUS           8

#define UI_BOTTOM_BAR_HEIGHT            44
#define UI_BOTTOM_BAR_WIDTH_PADDING     6
#define UI_BOTTOM_BAR_BOTTOM_PADDING    2
#define UI_BOTTOM_BAR_SIDE_PADDING      12
#define UI_BUTTON_WIDTH                 70
#define UI_BUTTON_HEIGHT                28

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

static lv_coord_t toChartCoord(float v);

static void computeYRangeFromProfile(const float *profile, uint32_t count, float *outMin, float *outMax);

static const char *formatTime(uint32_t totalSecs, char *buf, size_t bufSize);

static lv_obj_t *makeMenuOption(lv_obj_t *parent, const char *text);

/**
 * @brief Populate or update menu options for a vertical menu
 * 
 * @param vertMenuUi Pointer to the vertical menu UI structure
 * @param optionLabels Array of option labels. Each label is a \0 terminated string with a 32
 * character max length. If NULL, dummy labels will be used.
 * @param optionQty Number of options
 * 
 * @note This function handles both creating new options and updating existing ones.
 * If the option count changes, it will recreate all options. Assumes LVGL lock is held.
 */
static void populateVerticalMenuOptions(ST_VerticalMenuUi *vertMenuUi, const char **optionLabels, int optionQty);

/**
 * @brief Create a Vertical Menu Ui object
 * 
 * @param vertMenuUi Pointer to the vertical menu UI structure to initialize
 * @param menuTitle Title of the menu. A \0 terminated string with a 32 character max length
 * @param optionLabels Array of option labels. Each label is a \0 terminated string with a 32
 * character max length. If NULL, dummy labels will be used.
 * @param optionQty Number of options
 */
static void createVerticalMenuUi(ST_VerticalMenuUi *vertMenuUi, const char *menuTitle, const char **optionLabels, int optionQty);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "DISPLAY";

static esp_lcd_panel_io_handle_t ioHandle = NULL;
static esp_lcd_panel_handle_t panelHandle = NULL;
static lv_obj_t *mainLabel = NULL;
static ST_RoastChartUi roastChartUi = {0};
static ST_VerticalMenuUi mainMenuUi = {0};
static ST_VerticalMenuUi selectRoastMenuUi = {0};
static ST_PopupUi confirmationPopupUi = {0};

static const char *menuLabels[] = {
    "Roast",
    "Manage Roast Curves",
    "Connect"
};

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void display_lcdInit(void)
{
    esp_lcd_panel_io_spi_config_t ioConfig = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = LCD_MAX_CLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(DEFAULT_SPI_HOST, &ioConfig, &ioHandle));

    // Create ILI9341 panel
    esp_lcd_panel_dev_config_t panelConfig = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
#if USE_LCD_ILI9341
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(ioHandle, &panelConfig, &panelHandle));
#else
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(ioHandle, &panelConfig, &panelHandle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panelHandle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panelHandle, false, false));  // landscape if needed
#if USE_LCD_ILI9341
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panelHandle, true));
#endif

    // Backlight
    if (PIN_LED >= 0) {
        ledc_timer_config_t timerConfig = {
            .speed_mode = LCD_BCKLIGHT_MODE,
            .duty_resolution = LCD_BCKLIGHT_DUTY_RES,
            .timer_num = LCD_BCKLIGHT_TIMER,
            .freq_hz = 5000,
            .clk_cfg = LEDC_AUTO_CLK
        };
        ledc_channel_config_t channelConfig = {
            .gpio_num = PIN_LED,
            .speed_mode = LCD_BCKLIGHT_MODE,
            .channel = LCD_BCKLIGHT_CHANNEL,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LCD_BCKLIGHT_TIMER,
            .duty = 0,
            .hpoint = 0
        };
        ledc_timer_config(&timerConfig);
        ledc_channel_config(&channelConfig);
        unsigned long maxDuty = (1u << LCD_BCKLIGHT_DUTY_RES) - 1;
        unsigned long duty = (unsigned long)((INITIAL_BCKLIGHT_DUTY_PERCENT / 100.0f) *
                                             (float)maxDuty + 0.5f);
        ledc_set_duty(LCD_BCKLIGHT_MODE, LCD_BCKLIGHT_CHANNEL, duty);
        ledc_update_duty(LCD_BCKLIGHT_MODE, LCD_BCKLIGHT_CHANNEL);
    }
}

extern void display_uiInit(void)
{
    lv_init();

    lvgl_port_cfg_t portCfg = {
        .task_priority     = 4,
        .task_stack        = 8192,
        .task_affinity     = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms   = 5,
    };
    ESP_ERROR_CHECK(lvgl_port_init(&portCfg));

    lvgl_port_display_cfg_t dispCfg = {
        .io_handle      = ioHandle,
        .panel_handle   = panelHandle,   
        .buffer_size    = LCD_H_RES_IN_PIX * 40,
        .double_buffer  = true,           
        .hres           = LCD_H_RES_IN_PIX,     
        .vres           = LCD_V_RES_IN_PIX,     
        .monochrome     = false,         
        .color_format   = LV_COLOR_FORMAT_RGB565,  // LVGL uses RGB internally
        .rotation = {                    
            .swap_xy   = true,           
            .mirror_x  = false,           
            .mirror_y  = false
        },
        .flags = {
            .buff_dma    = true,         
            .buff_spiram = false         
        }
    };

    lv_disp_t *display = lvgl_port_add_disp(&dispCfg);
    assert(display);

    // NOTE: To change the rotation dinamically, use:
    // lv_disp_set_rotation(display, LV_DISPLAY_ROTATION_90);

    vTaskDelay(pdMS_TO_TICKS(200));
}

extern void display_setDispBrightness(float brightness)
{
    if (brightness < 0.0)
        brightness = 0;
    if (brightness > 100)
        brightness = 100;
    
    unsigned long maxDuty = (1u << LCD_BCKLIGHT_DUTY_RES) - 1;
    unsigned long duty = (unsigned long)((brightness / 100.0f) * (float)maxDuty + 0.5f);
    ledc_set_duty(LCD_BCKLIGHT_MODE, LCD_BCKLIGHT_CHANNEL, duty);
    ledc_update_duty(LCD_BCKLIGHT_MODE, LCD_BCKLIGHT_CHANNEL);
}

extern void display_setUiText(const char* text)
{
    lv_label_set_text(mainLabel, text);
    lv_obj_set_style_text_font(mainLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(mainLabel);
}

extern void display_uiStatusBarUpdate(bool ethUp, float tempC, int pwm, bool fault, const char *timeStr)
{
    static lv_obj_t *bar = NULL;
    static lv_obj_t *labelEth = NULL;
    static lv_obj_t *labelTemp = NULL;
    static lv_obj_t *labelPwm = NULL;
    static lv_obj_t *labelFault = NULL;
    static lv_obj_t *labelClock = NULL;

    const lv_color_t okColor    = lv_palette_main(LV_PALETTE_GREEN);
    const lv_color_t warnColor  = lv_palette_main(LV_PALETTE_YELLOW);
    const lv_color_t badColor   = lv_palette_main(LV_PALETTE_RED);
    const lv_color_t textColor  = lv_color_white();
    const lv_color_t bgColor    = lv_color_black();

    lvgl_port_lock(0);
    if (!bar)
    {
        // Container (top bar)
        bar = lv_obj_create(lv_screen_active());
        lv_obj_set_size(bar, lv_pct(100), 24);
        lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, bgColor, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_row(bar, 0, 0);
        lv_obj_set_style_pad_column(bar, 12, 0);
        lv_obj_set_style_pad_left(bar, 8, 0);
        lv_obj_set_style_pad_right(bar, 8, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);

        // Container type flex
        lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Left group: ETH, TEMP, SD
        labelEth   = lv_label_create(bar);
        labelTemp  = lv_label_create(bar);
        labelPwm    = lv_label_create(bar);

        // Spacer (flex pushes next ones to the right edge)
        lv_obj_t *spacer = lv_obj_create(bar);
        lv_obj_set_size(spacer, 1, 1);
        lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
        lv_obj_set_flex_grow(spacer, 1);

        // Right group: FAULT, CLOCK
        labelFault = lv_label_create(bar);
        labelClock = lv_label_create(bar);

        lv_obj_set_style_text_color(labelEth,   textColor, 0);
        lv_obj_set_style_text_color(labelTemp,  textColor, 0);
        lv_obj_set_style_text_color(labelPwm,    textColor, 0);
        lv_obj_set_style_text_color(labelFault, textColor, 0);
        lv_obj_set_style_text_color(labelClock, textColor, 0);
    }

    // --- Update ETH ---
    {
        const char *icon = ethUp ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE;
        lv_label_set_text_fmt(labelEth, "%s ETH", icon);
        lv_obj_set_style_text_color(labelEth, ethUp ? okColor : badColor, 0);
    }

    // --- Update TEMP ---
    {
        lv_label_set_text_fmt(labelTemp, " %d.%d°C", (int)tempC, (int)((tempC - (int)tempC) * 10));
        lv_color_t c = (tempC < 0.f || tempC > 100.f) ? warnColor : textColor;
        lv_obj_set_style_text_color(labelTemp, c, 0);
    }

    // --- Update motor PWM  ---
    {
        lv_label_set_text_fmt(labelPwm, LV_SYMBOL_CHARGE " %d%%", pwm);
        lv_color_t c = (pwm > 70) ? warnColor : textColor;
        lv_obj_set_style_text_color(labelPwm, c, 0);
    }

    // --- Update FAULT ---
    {
        if (fault) {
            lv_label_set_text(labelFault, LV_SYMBOL_WARNING " FAULT");
            lv_obj_set_style_text_color(labelFault, badColor, 0);
        } else {
            lv_label_set_text(labelFault, "");
        }
    }

    // --- Update CLOCK (optional string, e.g., "12:34") ---
    {
        lv_label_set_text(labelClock, (timeStr && *timeStr) ? timeStr : "");
        lv_obj_set_style_text_color(labelClock, textColor, 0);
    }
    lvgl_port_unlock();
}

extern void display_createRoastChart(const float *profile, uint32_t totalPoints, float yMin, float yMax)
{
    if (totalPoints == 0 || totalPoints > MAX_ROAST_TIME_IN_MIN)
        totalPoints = MAX_ROAST_TIME_IN_MIN;

    bool ok = lvgl_port_lock(0);
    if (!ok) { return; }

    roastChartUi.totalPoints = totalPoints;

    if (isfinite(yMin) && isfinite(yMax) && (yMax > yMin))
    {
        roastChartUi.yMin = yMin;
        roastChartUi.yMax = yMax;
    }
    else
    {
        computeYRangeFromProfile(profile, totalPoints, &roastChartUi.yMin, &roastChartUi.yMax);
    }

    // If chart already exists, just update the data
    if (roastChartUi.screen != NULL && roastChartUi.chart != NULL)
    {
        // Update chart point count if needed
        lv_chart_set_point_count(roastChartUi.chart, totalPoints);
        
        // Update Y and X ranges
        lv_chart_set_range(roastChartUi.chart, LV_CHART_AXIS_PRIMARY_Y, toChartCoord(roastChartUi.yMin), toChartCoord(roastChartUi.yMax));
        lv_chart_set_range(roastChartUi.chart, LV_CHART_AXIS_PRIMARY_X, toChartCoord(0), toChartCoord(totalPoints - 1));
        
        // Update target profile data
        for (uint32_t m = 0; m < totalPoints; ++m)
        {
            float v = isfinite(profile[m]) ? profile[m] : NAN;
            lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesTarget, m, isfinite(v) ? toChartCoord(v) : LV_CHART_POINT_NONE);
        }
        
        // Reset current series
        for (uint32_t m = 0; m < totalPoints; ++m)
        {
            lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesCurrent, m, LV_CHART_POINT_NONE);
        }
        
        // Search for labels and update them
        uint32_t child_count = lv_obj_get_child_cnt(roastChartUi.screen);
        for (uint32_t i = 0; i < child_count; i++)
        {
            lv_obj_t *child = lv_obj_get_child(roastChartUi.screen, i);
            if (lv_obj_check_type(child, &lv_label_class))
            {
                const char *text = lv_label_get_text(child);
                // Check if it's an axis label by looking at alignment
                lv_align_t align = lv_obj_get_style_align(child, LV_PART_MAIN);
                if (align == LV_ALIGN_OUT_LEFT_BOTTOM)
                {
                    lv_label_set_text_fmt(child, "%d", (int)roastChartUi.yMin);
                }
                else if (align == LV_ALIGN_OUT_LEFT_TOP)
                {
                    lv_label_set_text_fmt(child, "%d", (int)roastChartUi.yMax);
                }
                else if (text && strstr(text, "min") != NULL && strstr(text, "0") != NULL)
                {
                    lv_label_set_text(child, "0min");
                }
                else if (text && strstr(text, "min") != NULL)
                {
                    lv_label_set_text_fmt(child, "%" PRIu32 "min", (totalPoints ? totalPoints - 1 : 0));
                }
            }
        }
        
        // Reset info label
        lv_label_set_text(roastChartUi.labelInfo, "Tnow: --.-°C | Ttgt: --.-°C | Left: --:--");
        
        lv_obj_invalidate(roastChartUi.chart);

        lvgl_port_unlock();
        return;
    }

    // Screen
    roastChartUi.screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(roastChartUi.screen, lv_palette_main(LV_PALETTE_NONE), 0);  // Light brown/tan (RGB)
    lv_obj_set_style_bg_opa(roastChartUi.screen, LV_OPA_COVER, 0);

    // Chart
    roastChartUi.chart = lv_chart_create(roastChartUi.screen);
    lv_obj_set_size(roastChartUi.chart, lv_pct(UI_CHART_WIDTH_PCT), lv_pct(UI_CHART_HEIGHT_PCT));
    lv_obj_align(roastChartUi.chart, LV_ALIGN_TOP_MID, 0, UI_CHART_CENTER_Y_OFFSET);
    lv_chart_set_type(roastChartUi.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(roastChartUi.chart, LV_CHART_UPDATE_MODE_SHIFT); // we will address by index anyway
    lv_chart_set_point_count(roastChartUi.chart, totalPoints);

    // Y range and tick marks
    lv_chart_set_range(roastChartUi.chart, LV_CHART_AXIS_PRIMARY_Y, toChartCoord(roastChartUi.yMin), toChartCoord(roastChartUi.yMax));
    lv_chart_set_range(roastChartUi.chart, LV_CHART_AXIS_PRIMARY_X, toChartCoord(0), toChartCoord(totalPoints - 1));
    lv_chart_set_div_line_count(roastChartUi.chart, UI_CHART_DIV_LINES, UI_CHART_DIV_LINES);

    // Axis labels (simple min/max markers)
    lv_obj_t *yMinLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text_fmt(yMinLbl, "%d", (int)roastChartUi.yMin);
    ESP_LOGI(TAG, "YMin: %.1f", roastChartUi.yMin);
    lv_obj_align_to(yMinLbl, roastChartUi.chart, LV_ALIGN_OUT_LEFT_BOTTOM, UI_CHART_AXIS_LABEL_X_OFFSET, 0);

    lv_obj_t *yMaxLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text_fmt(yMaxLbl, "%d", (int)roastChartUi.yMax);
    ESP_LOGI(TAG, "YMax: %.1f", roastChartUi.yMax);
    lv_obj_align_to(yMaxLbl, roastChartUi.chart, LV_ALIGN_OUT_LEFT_TOP, UI_CHART_AXIS_LABEL_X_OFFSET, 0);

    // X-axis min/max (0 min ... totalPoints-1)
    lv_obj_t *xMinLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text(xMinLbl, "0min");
    lv_obj_align_to(xMinLbl, roastChartUi.chart, LV_ALIGN_OUT_BOTTOM_LEFT, 0, UI_CHART_AXIS_LABEL_Y_OFFSET);

    lv_obj_t *xMaxLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text_fmt(xMaxLbl, "%" PRIu32 "min", (totalPoints ? totalPoints - 1 : 0));
    lv_obj_align_to(xMaxLbl, roastChartUi.chart, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, UI_CHART_AXIS_LABEL_Y_OFFSET);

    // Series - use standard RGB colors
    roastChartUi.seriesTarget  = lv_chart_add_series(roastChartUi.chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    roastChartUi.seriesCurrent = lv_chart_add_series(roastChartUi.chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    // Load target profile points
    for (uint32_t m = 0; m < totalPoints; ++m) {
        float v = isfinite(profile[m]) ? profile[m] : NAN;
        lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesTarget, m, isfinite(v) ? toChartCoord(v) : LV_CHART_POINT_NONE);
    }
    // Initialize current with "no data"
    for (uint32_t m = 0; m < totalPoints; ++m) {
        lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesCurrent, m, LV_CHART_POINT_NONE);
    }

    // Title
    lv_obj_t *title = lv_label_create(roastChartUi.screen);
    lv_label_set_text(title, "Roast Profile (°C)");
    lv_obj_set_style_text_font(title, lv_theme_get_font_large(roastChartUi.screen), 0);
    lv_obj_align(title, LV_ALIGN_OUT_TOP_LEFT, UI_CHART_TITLE_X_OFFSET, -1);

    // Legend labels
    lv_obj_t *legend = lv_label_create(roastChartUi.screen);
    lv_label_set_text(legend, "#FF8000 Target#  #0080FF Current#");  // RGB format
    lv_label_set_recolor(legend, true);
    lv_obj_align_to(legend, roastChartUi.chart, LV_ALIGN_OUT_TOP_RIGHT, 0, UI_CHART_LEGEND_Y_OFFSET);

    // Info line
    roastChartUi.labelInfo = lv_label_create(roastChartUi.screen);
    lv_label_set_text(roastChartUi.labelInfo, "Tnow: --.-°C | Ttgt: --.-°C | Left: --:--");
    lv_obj_align_to(roastChartUi.labelInfo, roastChartUi.chart, LV_ALIGN_OUT_BOTTOM_MID, 0, UI_CHART_INFO_Y_OFFSET);

    // Bottom bar with fixed buttons
    lv_style_reset(&roastChartUi.styleBottomBar);
    lv_style_init(&roastChartUi.styleBottomBar);
    lv_style_set_bg_opa(&roastChartUi.styleBottomBar, LV_OPA_0);
    lv_style_set_bg_color(&roastChartUi.styleBottomBar, lv_palette_main(LV_PALETTE_NONE));
    lv_style_set_border_width(&roastChartUi.styleBottomBar, 0);
    lv_style_set_pad_hor(&roastChartUi.styleBottomBar, UI_BOTTOM_BAR_WIDTH_PADDING);
    lv_style_set_pad_bottom(&roastChartUi.styleBottomBar, UI_BOTTOM_BAR_BOTTOM_PADDING);

    roastChartUi.bottomBar = lv_obj_create(roastChartUi.screen);
    lv_obj_add_style(roastChartUi.bottomBar, &roastChartUi.styleBottomBar, 0);
    lv_obj_set_width(roastChartUi.bottomBar, lv_pct(100));
    lv_obj_set_height(roastChartUi.bottomBar, UI_BOTTOM_BAR_HEIGHT);
    lv_obj_align(roastChartUi.bottomBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(roastChartUi.bottomBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(roastChartUi.bottomBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    roastChartUi.btnReturn = lv_btn_create(roastChartUi.bottomBar);
    lv_obj_set_size(roastChartUi.btnReturn, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblRet = lv_label_create(roastChartUi.btnReturn);
    lv_label_set_text(lblRet, LV_SYMBOL_CLOSE);
    lv_obj_center(lblRet);

    lvgl_port_unlock();
}

extern void display_showRoastChart(void)
{
    if (roastChartUi.screen != NULL)
    {
        bool ok = lvgl_port_lock(0);
        if (!ok) { return; }
        lv_scr_load(roastChartUi.screen);
        lvgl_port_unlock();
    }
}

extern void display_updateRoastChart(float currTargetTemp, uint32_t elapsedSecs, float currentTemp)
{
    if (NULL == roastChartUi.screen) return;

    uint32_t minuteIdx = elapsedSecs / 60;
    if (minuteIdx >= (roastChartUi.totalPoints - 1)) minuteIdx = (roastChartUi.totalPoints - 1);

    bool ok = lvgl_port_lock(0);
    if (!ok) return;

    // Plot/overwrite current temp at this minute
    if ((elapsedSecs % 60) == 0)
    {
        lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesCurrent, minuteIdx,
                                 isfinite(currentTemp) ? toChartCoord(currentTemp) : LV_CHART_POINT_NONE);
    }

    // Compose info label
    char leftBuf[16];
    uint32_t totalSecs = ((roastChartUi.totalPoints - 1) * 60);
    uint32_t remainingSecs = (elapsedSecs >= totalSecs) ? 0 : (totalSecs - elapsedSecs);
    formatTime(remainingSecs, leftBuf, sizeof leftBuf);

    // If you keep target array available, replace "--.-" with its value:
    // e.g., float tgt = tempProfile[minuteIdx];
    // Here we’ll show "--.-" unless you wire it in.
    char info[96];
    snprintf(info, sizeof(info),
             "Tnow: %.1f°C | Ttgt: %.1f°C | Left: %s",
             isfinite(currentTemp) ? currentTemp : NAN,
             currTargetTemp,
             leftBuf);
    lv_label_set_text(roastChartUi.labelInfo, info);
    lv_obj_align_to(roastChartUi.labelInfo, roastChartUi.chart, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    // Optional: keep the chart view scrolled to show progress if totalPoints is large
    // lv_chart_set_zoom_x(roastChartUi.chart, zoomValue); // if you want zooming behaviour

    lv_obj_invalidate(roastChartUi.chart);
    lvgl_port_unlock();
}

extern void display_createMainMenu()
{
    createVerticalMenuUi(&mainMenuUi, "Main Menu", menuLabels, MAIN_MENU_OPTION_COUNT);
}

extern void display_applySelectionStyle(ST_VerticalMenuUi *ui, int optionQty)
{
    bool ok = lvgl_port_lock(0);
    if (!ok) {
        ESP_LOGW(TAG, "Failed to acquire LVGL lock for style update");
        return;
    }
    
    for (int i = 0; i < optionQty; ++i)
    {
        lv_obj_t *b = ui->menuOptions[i];
        if (i == ui->selectedIndex)
        {
            lv_obj_add_style(b, &ui->styleItemSelected, 0);
            lv_obj_remove_style(b, &ui->styleItem, 0);
            lv_obj_scroll_to_view_recursive(b, LV_ANIM_ON);
        }
        else
        {
            lv_obj_add_style(b, &ui->styleItem, 0);
            lv_obj_remove_style(b, &ui->styleItemSelected, 0);
        }
    }
    
    lvgl_port_unlock();
}

extern void display_showMainMenu(const char **optionLabels, int optionQty)
{
    if ((optionLabels != NULL) && (optionQty > 0))
    {
        populateVerticalMenuOptions(&mainMenuUi, optionLabels, optionQty);
    }

    if (mainMenuUi.screen != NULL) {
        bool ok = lvgl_port_lock(0);
        if (!ok) { return; }
        lv_scr_load(mainMenuUi.screen);
        lvgl_port_unlock();
    }
}

extern void display_updateBottomBarButton(lv_obj_t *btn, const char *label)
{
    if (btn == NULL) return;

    bool ok = lvgl_port_lock(0);
    if (!ok) { return; }

    lv_obj_t *lbl = lv_obj_get_child(btn, 0); // Assuming label is the first child
    if (lbl != NULL && lv_obj_check_type(lbl, &lv_label_class))
    {
        lv_label_set_text(lbl, label);
        lv_obj_center(lbl);
    }

    lvgl_port_unlock();
}

extern void display_setVerticalMenuTitle(ST_VerticalMenuUi *ui, const char *title)
{
    if (ui == NULL) return;

    if (strlen(title) > 32)
    {
        ESP_LOGW(TAG, "Menu title too long, truncating to 32 characters");
    }

    bool ok = lvgl_port_lock(0);
    if (!ok) { return; }

    lv_obj_t *titleObj = lv_obj_get_child(ui->screen, 0); // Assuming title is the first child
    if (titleObj == NULL || !lv_obj_check_type(titleObj, &lv_label_class))
    {
        ESP_LOGW(TAG, "Menu title object not found or invalid");
        lvgl_port_unlock();
        return;
    }
    
    lv_label_set_text(titleObj, title);
    lv_obj_set_style_text_font(titleObj, lv_theme_get_font_large(ui->screen), 0);
    lv_obj_align(titleObj, LV_ALIGN_TOP_MID, 0, UI_TITLE_Y_OFFSET_SMALL);

    lvgl_port_unlock();
}

extern ST_VerticalMenuUi *display_getMainMenuUi(void)
{
    return &mainMenuUi;
}

extern void display_createSelectRoastMenu(void)
{
    createVerticalMenuUi(&selectRoastMenuUi, SELECT_ROAST_MENU_TITLE, NULL, nvs_getProfileCount());
}

extern bool display_showSelectRoastMenu(ST_storedChart *charts, uint8_t chartCount)
{
    if (chartCount > MAX_VERTICAL_MENU_OPTION_COUNT)
    {
        ESP_LOGW(TAG, "Chart count (%d) exceeds max menu options (%d)", chartCount, MAX_VERTICAL_MENU_OPTION_COUNT);
        chartCount = MAX_VERTICAL_MENU_OPTION_COUNT;
    }

    if (chartCount == 0)
    {
        ESP_LOGW(TAG, "No charts to display");
        return false;
    }

    if (selectRoastMenuUi.screen != NULL)
    {
        const char *optionLabels[MAX_VERTICAL_MENU_OPTION_COUNT];
        
        for (int i = 0; i < chartCount; ++i)
        {
            optionLabels[i] = charts[i].chartName;
        }
        
        populateVerticalMenuOptions(&selectRoastMenuUi, optionLabels, chartCount);

        int ok = lvgl_port_lock(0);
        if (!ok) { return false; }
        lv_scr_load(selectRoastMenuUi.screen);
        lvgl_port_unlock();
    }

    return true;
}

extern ST_VerticalMenuUi *display_getSelectRoastMenuUi(void)
{
    return &selectRoastMenuUi;
}

extern void display_createConfirmationPopup(const char *message, lv_color_t currScrBckgnd)
{
    bool ok = lvgl_port_lock(0);
    if (!ok) {
        ESP_LOGE(TAG, "[X] Failed to acquire LVGL lock");
        return;
    }

    // If popup already exists, just update the message
    if (confirmationPopupUi.overlay != NULL)
    {
        if (confirmationPopupUi.messageLabel != NULL)
        {
            lv_label_set_text(confirmationPopupUi.messageLabel, message);
        }

        if (confirmationPopupUi.bottomBar != NULL)
        {
            lv_style_set_bg_color(&confirmationPopupUi.styleBottomBar, currScrBckgnd);
            lv_obj_add_style(confirmationPopupUi.bottomBar, &confirmationPopupUi.styleBottomBar, 0);
        }
        
        lvgl_port_unlock();
        return;
    }

    // Create semi-transparent overlay background as a new screen layer
    confirmationPopupUi.overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(confirmationPopupUi.overlay, LCD_H_RES_IN_PIX, LCD_V_RES_IN_PIX);
    lv_obj_set_pos(confirmationPopupUi.overlay, 0, 0);
    lv_obj_set_style_bg_color(confirmationPopupUi.overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(confirmationPopupUi.overlay, LV_OPA_0, 0);
    lv_obj_set_style_border_width(confirmationPopupUi.overlay, 0, 0);
    lv_obj_set_style_pad_all(confirmationPopupUi.overlay, 0, 0);
    lv_obj_set_style_radius(confirmationPopupUi.overlay, 0, 0);
    lv_obj_clear_flag(confirmationPopupUi.overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(confirmationPopupUi.overlay, LV_OBJ_FLAG_HIDDEN); // Start hidden

    // Create white balloon container
    lv_style_reset(&confirmationPopupUi.styleContainer);
    lv_style_init(&confirmationPopupUi.styleContainer);
    lv_style_set_bg_color(&confirmationPopupUi.styleContainer, lv_color_white());
    lv_style_set_bg_opa(&confirmationPopupUi.styleContainer, LV_OPA_COVER);
    lv_style_set_border_width(&confirmationPopupUi.styleContainer, UI_POPUP_CONTAINER_BORDER);
    lv_style_set_border_color(&confirmationPopupUi.styleContainer, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_radius(&confirmationPopupUi.styleContainer, UI_POPUP_CONTAINER_RADIUS);
    lv_style_set_pad_all(&confirmationPopupUi.styleContainer, UI_POPUP_CONTAINER_PADDING);
    lv_style_set_shadow_width(&confirmationPopupUi.styleContainer, UI_POPUP_SHADOW_WIDTH);
    lv_style_set_shadow_opa(&confirmationPopupUi.styleContainer, LV_OPA_30);

    confirmationPopupUi.container = lv_obj_create(confirmationPopupUi.overlay);
    lv_obj_add_style(confirmationPopupUi.container, &confirmationPopupUi.styleContainer, 0);
    lv_obj_set_size(confirmationPopupUi.container, lv_pct(UI_POPUP_CONTAINER_WIDTH_PCT), LV_SIZE_CONTENT);
    lv_obj_center(confirmationPopupUi.container);
    lv_obj_set_flex_flow(confirmationPopupUi.container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(confirmationPopupUi.container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(confirmationPopupUi.container, LV_OBJ_FLAG_SCROLLABLE);

    // Message label
    confirmationPopupUi.messageLabel = lv_label_create(confirmationPopupUi.container);
    lv_label_set_text(confirmationPopupUi.messageLabel, message);
    lv_label_set_long_mode(confirmationPopupUi.messageLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(confirmationPopupUi.messageLabel, lv_pct(100));
    lv_obj_set_style_text_color(confirmationPopupUi.messageLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(confirmationPopupUi.messageLabel, LV_TEXT_ALIGN_CENTER, 0);

    // Bottom button bar (overlaid on top of screen's bottom bar)
    lv_style_reset(&confirmationPopupUi.styleBottomBar);
    lv_style_init(&confirmationPopupUi.styleBottomBar);
    lv_style_set_bg_opa(&confirmationPopupUi.styleBottomBar, LV_OPA_COVER);
    lv_style_set_bg_color(&confirmationPopupUi.styleBottomBar, currScrBckgnd);
    lv_style_set_border_width(&confirmationPopupUi.styleBottomBar, 0);
    lv_style_set_pad_hor(&confirmationPopupUi.styleBottomBar, UI_BOTTOM_BAR_WIDTH_PADDING);
    lv_style_set_pad_bottom(&confirmationPopupUi.styleBottomBar, UI_BOTTOM_BAR_BOTTOM_PADDING);

    confirmationPopupUi.bottomBar = lv_obj_create(confirmationPopupUi.overlay);
    lv_obj_add_style(confirmationPopupUi.bottomBar, &confirmationPopupUi.styleBottomBar, 0);
    lv_obj_set_width(confirmationPopupUi.bottomBar, lv_pct(100));
    lv_obj_set_height(confirmationPopupUi.bottomBar, UI_BOTTOM_BAR_HEIGHT);
    lv_obj_align(confirmationPopupUi.bottomBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(confirmationPopupUi.bottomBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(confirmationPopupUi.bottomBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Cancel button (left)
    confirmationPopupUi.btnCancel = lv_btn_create(confirmationPopupUi.bottomBar);
    lv_obj_set_size(confirmationPopupUi.btnCancel, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblCancel = lv_label_create(confirmationPopupUi.btnCancel);
    lv_label_set_text(lblCancel, LV_SYMBOL_CLOSE);
    lv_obj_center(lblCancel);

    // Confirm button (right)
    confirmationPopupUi.btnConfirm = lv_btn_create(confirmationPopupUi.bottomBar);
    lv_obj_set_size(confirmationPopupUi.btnConfirm, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblConfirm = lv_label_create(confirmationPopupUi.btnConfirm);
    lv_label_set_text(lblConfirm, LV_SYMBOL_OK);
    lv_obj_center(lblConfirm);

    lvgl_port_unlock();
}

extern void display_showConfirmationPopup(void)
{
    if (confirmationPopupUi.overlay != NULL) {
        bool ok = lvgl_port_lock(0);
        if (!ok) { return; }
        lv_obj_clear_flag(confirmationPopupUi.overlay, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }
}

extern void display_hideConfirmationPopup(void)
{
    if (confirmationPopupUi.overlay != NULL) {
        bool ok = lvgl_port_lock(0);
        if (!ok) { return; }
        lv_obj_add_flag(confirmationPopupUi.overlay, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static lv_coord_t toChartCoord(float v)
{
    // lv_coord_t is typically int16_t; clamp to safe range
    if (v > 32760.0f) v = 32760.0f;
    if (v < -32760.0f) v = -32760.0f;
    return (lv_coord_t)lrintf(v);
}

static void computeYRangeFromProfile(const float *profile, uint32_t count, float *outMin, float *outMax)
{
    float mn = 1e9f, mx = -1e9f;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (isfinite(profile[i]))
        {
            if (profile[i] < mn) mn = profile[i];
            if (profile[i] > mx) mx = profile[i];
        }
    }
    if (!isfinite(mn) || !isfinite(mx)) { mn = 0.f; mx = 250.f; }
    // Add some headroom
    float pad = fmaxf(5.f, 0.08f * (mx - mn));
    *outMin = floorf(mn - pad);
    *outMax = ceilf(mx + pad);
    if (*outMax <= *outMin) { *outMin = 0.f; *outMax = *outMin + 10.f; }
}

static const char *formatTime(uint32_t totalSecs, char *buf, size_t bufSize)
{
    uint32_t mins = totalSecs / 60;
    uint32_t secs = totalSecs % 60;
    snprintf(buf, bufSize, "%02" PRIu32 ":%02" PRIu32, mins, secs);
    return buf;
}

static lv_obj_t *makeMenuOption(lv_obj_t *parent, const char *text)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_width(btn, lv_pct(UI_MENU_OPTION_WIDTH_PCT));
    lv_obj_set_height(btn, UI_MENU_OPTION_HEIGHT);
    lv_obj_set_style_radius(btn, UI_MENU_OPTION_RADIUS, 0);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

static void populateVerticalMenuOptions(ST_VerticalMenuUi *vertMenuUi, const char **optionLabels, int optionQty)
{
    if (optionQty > MAX_VERTICAL_MENU_OPTION_COUNT)
    {
        ESP_LOGE(TAG, "Option quantity (%d) exceeds max (%d)", optionQty, MAX_VERTICAL_MENU_OPTION_COUNT);
        optionQty = MAX_VERTICAL_MENU_OPTION_COUNT;
    }

    // Count existing options
    int existingCount = 0;
    for (int i = 0; i < MAX_VERTICAL_MENU_OPTION_COUNT; ++i)
    {
        if (vertMenuUi->menuOptions[i] != NULL)
        {
            existingCount++;
        }
        else
        {
            break;
        }
    }
    
    // Check if we need to recreate options (different count)
    bool needsRecreate = (existingCount != optionQty);

    bool ok = lvgl_port_lock(0);
    if (!ok) { return; }
    
    if (needsRecreate)
    {
        // Delete old options
        for (int i = 0; i < MAX_VERTICAL_MENU_OPTION_COUNT; ++i)
        {
            if (vertMenuUi->menuOptions[i] != NULL)
            {
                lv_obj_del(vertMenuUi->menuOptions[i]);
                vertMenuUi->menuOptions[i] = NULL;
            }
        }
        
        // Create new options
        for (int i = 0; i < optionQty; ++i)
        {
            vertMenuUi->menuOptions[i] = makeMenuOption(vertMenuUi->menuContainer,
                                                        ((NULL != optionLabels) ? optionLabels[i] : "Dummy"));
            lv_obj_add_style(vertMenuUi->menuOptions[i], &vertMenuUi->styleItem, 0);
        }
    }
    else
    {
        // Just update existing labels
        for (int i = 0; i < optionQty; ++i)
        {
            if (vertMenuUi->menuOptions[i] != NULL)
            {
                lv_obj_t *label = lv_obj_get_child(vertMenuUi->menuOptions[i], 0);
                if (label != NULL && optionLabels != NULL)
                {
                    lv_label_set_text(label, optionLabels[i]);
                }
            }
        }
    }

    lvgl_port_unlock();
}

static void createVerticalMenuUi(ST_VerticalMenuUi *vertMenuUi, const char *menuTitle, const char **optionLabels, int optionQty)
{
    if (optionQty > MAX_VERTICAL_MENU_OPTION_COUNT)
    {
        ESP_LOGE(TAG, "Option quantity (%d) exceeds max (%d)", optionQty, MAX_VERTICAL_MENU_OPTION_COUNT);
        optionQty = MAX_VERTICAL_MENU_OPTION_COUNT;
    }

    bool ok = lvgl_port_lock(0);
    if (!ok)
    {
        ESP_LOGE(TAG, "[X] Failed to acquire LVGL lock");
        return;
    }

    // If screen already exists, clean it up first
    if (vertMenuUi->screen != NULL)
    {
        lv_obj_del(vertMenuUi->screen);
        vertMenuUi->screen = NULL;
    }

    vertMenuUi->selectedIndex = 0;

    lv_style_reset(&vertMenuUi->styleItem);
    lv_style_init(&vertMenuUi->styleItem);
    lv_style_set_bg_color(&vertMenuUi->styleItem, lv_palette_lighten(LV_PALETTE_GREY, 2));
    lv_style_set_bg_opa(&vertMenuUi->styleItem, LV_OPA_COVER);
    lv_style_set_border_width(&vertMenuUi->styleItem, 0);
    lv_style_set_pad_all(&vertMenuUi->styleItem, 0);

    lv_style_reset(&vertMenuUi->styleItemSelected);
    lv_style_init(&vertMenuUi->styleItemSelected);
    lv_style_set_bg_opa(&vertMenuUi->styleItemSelected, LV_OPA_COVER);
    lv_style_set_bg_grad_dir(&vertMenuUi->styleItemSelected, LV_GRAD_DIR_VER);
    lv_style_set_bg_color(&vertMenuUi->styleItemSelected, lv_palette_lighten(LV_PALETTE_BLUE, 1));
    lv_style_set_bg_grad_color(&vertMenuUi->styleItemSelected, lv_palette_darken(LV_PALETTE_BLUE, 1));
    lv_style_set_border_width(&vertMenuUi->styleItemSelected, 0);
    lv_style_set_pad_all(&vertMenuUi->styleItemSelected, 0);

    // Screen
    vertMenuUi->screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(vertMenuUi->screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(vertMenuUi->screen, LV_OPA_COVER, 0);

    // Title
    lv_obj_t *title = lv_label_create(vertMenuUi->screen);
    lv_label_set_text(title, menuTitle);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, lv_theme_get_font_large(vertMenuUi->screen), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_TITLE_Y_OFFSET_SMALL);

    // Menu container (center area) - scrollable, shows max 3 options at a time
    vertMenuUi->menuContainer = lv_obj_create(vertMenuUi->screen);
    lv_obj_set_size(vertMenuUi->menuContainer, lv_pct(UI_MENU_CONTAINER_WIDTH_PCT), lv_pct(UI_MENU_CONTAINER_HEIGHT_PCT));
    lv_obj_align(vertMenuUi->menuContainer, LV_ALIGN_TOP_MID, 0, UI_MENU_CONTAINER_Y_OFFSET);
    lv_obj_set_flex_flow(vertMenuUi->menuContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(vertMenuUi->menuContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(vertMenuUi->menuContainer, LV_OPA_TRANSP, 0);
    // Enable vertical scrolling
    lv_obj_set_scrollbar_mode(vertMenuUi->menuContainer, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(vertMenuUi->menuContainer, LV_DIR_VER);

    // Populate menu options
    populateVerticalMenuOptions(vertMenuUi, optionLabels, optionQty);

    // Bottom bar with 4 fixed buttons
    lv_style_reset(&vertMenuUi->styleBottomBar);
    lv_style_init(&vertMenuUi->styleBottomBar);
    lv_style_set_bg_opa(&vertMenuUi->styleBottomBar, LV_OPA_0);
    lv_style_set_bg_color(&vertMenuUi->styleBottomBar, lv_palette_main(LV_PALETTE_NONE));
    lv_style_set_border_width(&vertMenuUi->styleBottomBar, 0);
    lv_style_set_pad_hor(&vertMenuUi->styleBottomBar, UI_BOTTOM_BAR_WIDTH_PADDING);
    lv_style_set_pad_bottom(&vertMenuUi->styleBottomBar, 2);

    vertMenuUi->bottomBar = lv_obj_create(vertMenuUi->screen);
    lv_obj_add_style(vertMenuUi->bottomBar, &vertMenuUi->styleBottomBar, 0);
    lv_obj_set_width(vertMenuUi->bottomBar, lv_pct(100));
    lv_obj_set_height(vertMenuUi->bottomBar, UI_BOTTOM_BAR_HEIGHT);
    lv_obj_align(vertMenuUi->bottomBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(vertMenuUi->bottomBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vertMenuUi->bottomBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    vertMenuUi->btnReturn = lv_btn_create(vertMenuUi->bottomBar);
    lv_obj_set_size(vertMenuUi->btnReturn, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblRet = lv_label_create(vertMenuUi->btnReturn);
    lv_label_set_text(lblRet, LV_SYMBOL_LEFT);
    lv_obj_center(lblRet);

    vertMenuUi->btnUp = lv_btn_create(vertMenuUi->bottomBar);
    lv_obj_set_size(vertMenuUi->btnUp, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblUp = lv_label_create(vertMenuUi->btnUp);
    lv_label_set_text(lblUp, LV_SYMBOL_UP);
    lv_obj_center(lblUp);

    vertMenuUi->btnDown = lv_btn_create(vertMenuUi->bottomBar);
    lv_obj_set_size(vertMenuUi->btnDown, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblDown = lv_label_create(vertMenuUi->btnDown);
    lv_label_set_text(lblDown, LV_SYMBOL_DOWN);
    lv_obj_center(lblDown);

    vertMenuUi->btnSelect = lv_btn_create(vertMenuUi->bottomBar);
    lv_obj_set_size(vertMenuUi->btnSelect, UI_BUTTON_WIDTH, UI_BUTTON_HEIGHT);
    lv_obj_t *lblSel = lv_label_create(vertMenuUi->btnSelect);
    lv_label_set_text(lblSel, LV_SYMBOL_OK);
    lv_obj_center(lblSel);

    // Initial selection
    display_applySelectionStyle(vertMenuUi, optionQty);

    lvgl_port_unlock();
}