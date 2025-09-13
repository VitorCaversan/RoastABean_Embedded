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

#define LCD_MAX_CLK_HZ        40000000  // 40MHz

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "DISPLAY";

static esp_lcd_panel_io_handle_t ioHandle = NULL;
static esp_lcd_panel_handle_t panelHandle = NULL;

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

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello ILI9341!");
    lv_obj_center(label);
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/