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

#define DEBOUNCE_INTERVAL_US  200000 // 200ms

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

static void btnIsrHndlr(void *arg)
{
    const int pin = (int)(intptr_t)arg;
    
    ST_extEventMsg msg = {0};
    uint8_t pressedBtnIdx = 0;
    switch (pin)
    {
        case BTN_1_GPIO_PIN:
            pressedBtnIdx = 0;
            msg.event = EXT_EVENT_BTN_1_PRESSED;
            break;
        case BTN_2_GPIO_PIN:
            pressedBtnIdx = 1;
            msg.event = EXT_EVENT_BTN_2_PRESSED;
            break;
        case BTN_3_GPIO_PIN:
            pressedBtnIdx = 2;
            msg.event = EXT_EVENT_BTN_3_PRESSED;
            break;
        case BTN_4_GPIO_PIN:
            pressedBtnIdx = 3;
            msg.event = EXT_EVENT_BTN_4_PRESSED;
            break;
        default:
            // Unknown pin, should not happen
            return;
    }

#if DEBOUNCE_INTERVAL_US > 0
    unsigned long now = esp_timer_get_time();
    unsigned long dt  = now - lastIsrInUs[pressedBtnIdx];
    if (dt < DEBOUNCE_INTERVAL_US) {
        return;
    }
    lastIsrInUs[pressedBtnIdx] = now;
#else
    unsigned long now = esp_timer_get_time();
#endif
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

#if BUTTON_DEBUG
    if (pressedBtnIdx < BTN_QTY)
    {
        EN_buttons btnEvent = (EN_buttons)pressedBtnIdx;
        xQueueSendFromISR(OS_mainTaskQueue, &btnEvent, &xHigherPriorityTaskWoken);
    }
#else
    // Best effort enqueue; if queue is full, drop or handle later.
    xQueueSendFromISR(OS_btnHndlrsTaskQueue, &msg, &xHigherPriorityTaskWoken);
#endif

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
    gpio_intr_disable(BTN_1_GPIO_PIN);
    gpio_intr_disable(BTN_2_GPIO_PIN);
    gpio_intr_disable(BTN_3_GPIO_PIN);
    gpio_intr_disable(BTN_4_GPIO_PIN);

    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BTN_1_GPIO_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = 1ULL << BTN_2_GPIO_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = 1ULL << BTN_3_GPIO_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.pin_bit_mask = 1ULL << BTN_4_GPIO_PIN;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_1_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_1_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_2_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_2_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_3_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_3_GPIO_PIN));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_4_GPIO_PIN, btnIsrHndlr, (void*)(intptr_t)BTN_4_GPIO_PIN));

    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_intr_enable(BTN_1_GPIO_PIN);
    gpio_intr_enable(BTN_2_GPIO_PIN);
    gpio_intr_enable(BTN_3_GPIO_PIN);
    gpio_intr_enable(BTN_4_GPIO_PIN);

    ESP_LOGI(TAG, "Created buttons");
}