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

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "DISPLAY";

static esp_lcd_panel_io_handle_t ioHandle = NULL;
static esp_lcd_panel_handle_t panelHandle = NULL;
static lv_obj_t *mainLabel = NULL;

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

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/