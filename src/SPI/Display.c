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

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

static lv_coord_t toChartCoord(float v);

static void computeYRangeFromProfile(const float *profile, uint32_t count, float *outMin, float *outMax);

static const char *formatTime(uint32_t totalSecs, char *buf, size_t bufSize);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "DISPLAY";

static esp_lcd_panel_io_handle_t ioHandle = NULL;
static esp_lcd_panel_handle_t panelHandle = NULL;
static lv_obj_t *mainLabel = NULL;
static ST_RoastChartUi roastChartUi = {0};

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
        .task_stack        = 4096,
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
        .color_format   = LV_COLOR_FORMAT_RGB565,
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

    mainLabel = lv_label_create(lv_screen_active());
    lv_label_set_text(mainLabel, "Hello Alfons!");
    lv_obj_set_style_text_font(mainLabel, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(mainLabel);
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

    const lv_color_t okColor    = lv_color_hex(0x21C55E); // green-ish
    const lv_color_t warnColor  = lv_color_hex(0xEAB308); // amber
    const lv_color_t badColor   = lv_color_hex(0xEF4444); // red
    const lv_color_t textColor  = lv_color_white();
    const lv_color_t bgColor    = lv_color_hex(0x111827); // dark

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

extern void display_createRoastChart(const float *profile, uint32_t totalMins, float yMin, float yMax)
{
    if (totalMins == 0 || totalMins > MAX_ROAST_TIME_IN_MIN)
        totalMins = MAX_ROAST_TIME_IN_MIN;

    bool ok = lvgl_port_lock(0);
    if (!ok) { return; }

    roastChartUi.totalMins = totalMins;

    if (!(isfinite(yMin) && isfinite(yMax) && yMax > yMin)) {
        computeYRangeFromProfile(profile, totalMins, &roastChartUi.yMin, &roastChartUi.yMax);
    } else {
        roastChartUi.yMin = yMin;
        roastChartUi.yMax = yMax;
    }

    // Screen
    roastChartUi.screen = lv_obj_create(NULL);

    // Title
    lv_obj_t *title = lv_label_create(roastChartUi.screen);
    lv_label_set_text(title, "Roast Profile (°C)");
    lv_obj_set_style_text_font(title, lv_theme_get_font_large(roastChartUi.screen), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    // Chart
    roastChartUi.chart = lv_chart_create(roastChartUi.screen);
    lv_obj_set_size(roastChartUi.chart, lv_pct(96), lv_pct(70));
    lv_obj_align(roastChartUi.chart, LV_ALIGN_CENTER, 0, 10);
    lv_chart_set_type(roastChartUi.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(roastChartUi.chart, LV_CHART_UPDATE_MODE_SHIFT); // we will address by index anyway
    lv_chart_set_point_count(roastChartUi.chart, totalMins);

    // Y range and tick marks
    lv_chart_set_range(roastChartUi.chart, LV_CHART_AXIS_PRIMARY_Y, toChartCoord(roastChartUi.yMin), toChartCoord(roastChartUi.yMax));
    lv_chart_set_div_line_count(roastChartUi.chart, 6, 6);
    lv_obj_set_style_pad_left(roastChartUi.chart, 50, 0);   // room for Y labels
    lv_obj_set_style_pad_bottom(roastChartUi.chart, 30, 0); // room for X labels

    // Axis labels (simple min/max markers)
    lv_obj_t *yMinLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text_fmt(yMinLbl, "%.0f", roastChartUi.yMin);
    lv_obj_align_to(yMinLbl, roastChartUi.chart, LV_ALIGN_OUT_LEFT_BOTTOM, -4, 0);

    lv_obj_t *yMaxLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text_fmt(yMaxLbl, "%.0f", roastChartUi.yMax);
    lv_obj_align_to(yMaxLbl, roastChartUi.chart, LV_ALIGN_OUT_LEFT_TOP, -4, 0);

    // X-axis min/max (0 min ... totalMins-1)
    lv_obj_t *xMinLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text(xMinLbl, "0m");
    lv_obj_align_to(xMinLbl, roastChartUi.chart, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    lv_obj_t *xMaxLbl = lv_label_create(roastChartUi.screen);
    lv_label_set_text_fmt(xMaxLbl, "%" PRIu32 "m", (totalMins ? totalMins - 1 : 0));
    lv_obj_align_to(xMaxLbl, roastChartUi.chart, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 2);

    // Series
    roastChartUi.seriesTarget  = lv_chart_add_series(roastChartUi.chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    roastChartUi.seriesCurrent = lv_chart_add_series(roastChartUi.chart, lv_palette_main(LV_PALETTE_BLUE),   LV_CHART_AXIS_PRIMARY_Y);

    // Load target profile points
    for (uint32_t m = 0; m < totalMins; ++m) {
        float v = isfinite(profile[m]) ? profile[m] : NAN;
        lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesTarget, m, isfinite(v) ? toChartCoord(v) : LV_CHART_POINT_NONE);
    }
    // Initialize current with "no data"
    for (uint32_t m = 0; m < totalMins; ++m) {
        lv_chart_set_value_by_id(roastChartUi.chart, roastChartUi.seriesCurrent, m, LV_CHART_POINT_NONE);
    }

    // Legend labels
    lv_obj_t *legend = lv_label_create(roastChartUi.screen);
    lv_label_set_text(legend, "#ffa500 Target#  #0000ff Current#");
    lv_label_set_recolor(legend, true);
    lv_obj_align_to(legend, roastChartUi.chart, LV_ALIGN_OUT_TOP_RIGHT, 0, -4);

    // Info line
    roastChartUi.labelInfo = lv_label_create(roastChartUi.screen);
    lv_label_set_text(roastChartUi.labelInfo, "Tnow: --.-°C | Ttgt: --.-°C | Left: --:--");
    lv_obj_align(roastChartUi.labelInfo, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Load the screen (optional: or attach to existing)
    lv_scr_load(roastChartUi.screen);

    lvgl_port_unlock();
}

extern void display_updateRoastChart(float currTargetTemp, uint32_t elapsedSecs, float currentTemp)
{
    if (NULL == roastChartUi.screen) return;

    uint32_t minuteIdx = elapsedSecs / 60;
    if (minuteIdx >= roastChartUi.totalMins) minuteIdx = roastChartUi.totalMins - 1;

    // We can read back target from the chart series (or keep your tempProfile accessible)
    // If you prefer direct array access, expose tempProfile[] here.
    // Below we just read the plotted point (convert back from lv_coord_t).
    // NOTE: lv_chart_get_point_pos_by_id gives pixel pos, not value; so use your array if available.
    // For simplicity, pass target array from your context if you want exact numbers on the label.
    // Here we’ll just skip exact target value if not provided.

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
    uint32_t totalSecs = roastChartUi.totalMins * 60;
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

    // Optional: keep the chart view scrolled to show progress if totalMins is large
    // lv_chart_set_zoom_x(roastChartUi.chart, zoomValue); // if you want zooming behaviour

    lv_obj_invalidate(roastChartUi.chart);
    lvgl_port_unlock();
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