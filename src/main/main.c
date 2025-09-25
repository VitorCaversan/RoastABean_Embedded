/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "DCMotor.h"
#include "TempSens.h"
#include "SpiConfig.h"
#include "Display.h"
#include "Buttons.h"
#include "OSConfig.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

// Enable this config,  we will print debug formated string, which in return can be captured and parsed by Serial-Studio
#define SERIAL_STUDIO_DEBUG           CONFIG_SERIAL_STUDIO_DEBUG

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

static void mainTask(void *arg);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

QueueHandle_t OS_mainTaskQueue = NULL;

static const char *TAG = "MAIN";

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

void app_main(void)
{
    OS_mainTaskQueue = xQueueCreate(32, sizeof(EN_buttons));
    configASSERT(OS_mainTaskQueue != NULL);

    spiConfig_configureSpiBus();
    // display_lcdInit();
    // display_uiInit();
    // tempSens_init();
    DCMotor_initDcMotors();
    btn_configButtons();

    xTaskCreatePinnedToCore(mainTask, "mainTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);
}

static void mainTask(void *arg)
{
    ST_motorControlContext *motor_ctrl_ctx = DCMotor_getContextFromMotor(MOTOR_1);

    ESP_LOGI(TAG, "Enable motor forward");
    bdc_motor_enable(motor_ctrl_ctx->motor);
    bdc_motor_forward(motor_ctrl_ctx->motor);

    float temp = 0.0f;
    EN_buttons msg;

    while (1) {
        if (xQueueReceive(OS_mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg)
            {
            case BTN_1_PRESSED:
                ESP_LOGI(TAG, "Button 1 pressed");
                break;
            case BTN_2_PRESSED:
                ESP_LOGI(TAG, "Button 2 pressed");
                break;
            case BTN_3_PRESSED:
                ESP_LOGI(TAG, "Button 3 pressed");
                break;
            case BTN_4_PRESSED:
                ESP_LOGI(TAG, "Button 4 pressed");
                // temp = tempSens_getTemperature();

                // if (temp < 25.0f)
                // {
                //     ESP_LOGI(TAG, "Temperature is below 25°C");
                //     bdc_motor_set_speed(motor_ctrl_ctx->motor, 25);
                // }
                // else if (temp < 30.0f)
                // {
                //     ESP_LOGI(TAG, "Temperature is below 30°C");
                //     bdc_motor_set_speed(motor_ctrl_ctx->motor, 50);
                // }
                // else
                // {
                //     ESP_LOGI(TAG, "Temperature is above 30°C");
                //     bdc_motor_set_speed(motor_ctrl_ctx->motor, 75);
                // }
                break;
            default:
                ESP_LOGW(TAG, "Unknown button press: %d", msg);
                break;
            }
        }
        // the following logging format is according to the requirement of serial-studio frame format
        // also see the dashboard config file `serial-studio-dashboard.json` for more information
#if SERIAL_STUDIO_DEBUG
        printf("/*%d*/\r\n", motor_ctrl_ctx.report_pulses);
#endif
    }
}