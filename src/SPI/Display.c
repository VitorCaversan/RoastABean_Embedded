/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Display.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define PIN_CS       GPIO_NUM_16
#define PIN_DC       GPIO_NUM_18
#define PIN_RST      -1 // Not used
#define PIN_LED      GPIO_NUM_21

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

static void lvglFlushCallback(lv_display_t *display,
                              const lv_area_t *area,
                              uint8_t *pixelMap);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static esp_lcd_panel_handle_t panelHandle = NULL;

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void display_lcdInit(void)
{
    esp_lcd_panel_io_handle_t ioHandle;
    esp_lcd_panel_io_spi_config_t ioConfig = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = 40 * 1000 * 1000,
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(ioHandle, &panelConfig, &panelHandle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panelHandle, true, false));  // landscape if needed
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panelHandle, true));

    // Backlight
    if (PIN_LED >= 0) {
        ledc_timer_config_t timerConfig = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 5000,
            .clk_cfg = LEDC_AUTO_CLK
        };
        ledc_channel_config_t channelConfig = {
            .gpio_num = PIN_LED,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0
        };
        ledc_timer_config(&timerConfig);
        ledc_channel_config(&channelConfig);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 800);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
}

extern void display_uiInit(void)
{
    lv_init();

    // Create LVGL display buffer
    static uint16_t buf1[LCD_H_RES_IN_PIX * 40];
    static uint16_t buf2[LCD_H_RES_IN_PIX * 40];

    lv_display_t *display = lv_display_create(LCD_H_RES_IN_PIX, LCD_V_RES_IN_PIX);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, lvglFlushCallback);
    lv_display_set_buffers(display, buf1, buf2,
                           sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, panelHandle);

    // Attach LVGL task handler via esp_lvgl_port
    lvgl_port_cfg_t portConfig = ESP_LVGL_PORT_INIT_CONFIG();
    portConfig.task_priority = 4;
    portConfig.task_stack = 4096;
    ESP_ERROR_CHECK(lvgl_port_init(&portConfig));
    lvgl_port_add_disp(display);

    // Simple UI: create a label
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello ILI9341!");
    lv_obj_center(label);
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void lvglFlushCallback(lv_display_t *display,
                              const lv_area_t *area,
                              uint8_t *pixelMap)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(display);
    esp_lcd_panel_draw_bitmap(panel,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              pixelMap);
    lv_disp_flush_ready(display);
}