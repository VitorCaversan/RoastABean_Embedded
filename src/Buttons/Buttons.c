/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Buttons.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define BTN_1_GPIO_PIN        GPIO_NUM_33
#define BTN_2_GPIO_PIN        GPIO_NUM_34
#define BTN_3_GPIO_PIN        44
#define BTN_4_GPIO_PIN        43

#define DEBOUNCE_INTERVAL_US  50000 // 50ms

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static unsigned long lastIsrInUs[BTN_QTY] = {0};

static const char *TAG = "BUTTONS";

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void IRAM_ATTR btnIsrHndlr(void *arg)
{
    const int pin = (int)(intptr_t)arg;
    
    EN_buttons msg = BTN_1_PRESSED;
    switch (pin)
    {
        case BTN_1_GPIO_PIN:
            msg = BTN_1_PRESSED;
            break;
        case BTN_2_GPIO_PIN:
            msg = BTN_2_PRESSED;
            break;
        case BTN_3_GPIO_PIN:
            msg = BTN_3_PRESSED;
            break;
        case BTN_4_GPIO_PIN:
            msg = BTN_4_PRESSED;
            break;
        default:
            // Unknown pin, should not happen
            return;
    }

#if DEBOUNCE_INTERVAL_US > 0
    unsigned long now = esp_timer_get_time();
    unsigned long dt  = now - lastIsrInUs[msg];
    if (dt < DEBOUNCE_INTERVAL_US) {
        return;
    }
    lastIsrInUs[msg] = now;
#else
    unsigned long now = esp_timer_get_time();
#endif
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Best effort enqueue; if queue is full, drop or handle later.
    xQueueSendFromISR(OS_mainTaskQueue, &msg, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void btn_configButtons(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BTN_1_GPIO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = 1ULL << BTN_2_GPIO_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = 1ULL << BTN_3_GPIO_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = 1ULL << BTN_4_GPIO_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Install ISR service; use IRAM flag so handler runs from IRAM
    // (GPIO ISR service keeps handlers in IRAM when ESP_INTR_FLAG_IRAM is set)
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_1_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_1_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_2_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_2_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_3_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_3_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_4_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_4_GPIO_PIN));

    ESP_LOGI(TAG, "Created buttons");
}