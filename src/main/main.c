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
#include "TriacControl.h"
#include "OSConfig.h"
#include "PIDControl.h"
#include "BtnHndlrs.h"
#include "Bluetooth.h"
#include "NVShndlr.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

// Enable this config,  we will print debug formated string, which in return can be captured and parsed by Serial-Studio
#define SERIAL_STUDIO_DEBUG           CONFIG_SERIAL_STUDIO_DEBUG

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

#if BUTTON_DEBUG
static void mainTask(void *arg);
#endif

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

QueueHandle_t OS_mainTaskQueue = NULL;
QueueHandle_t OS_btnHndlrsTaskQueue = NULL;
QueueHandle_t OS_bleEventQueue = NULL;
QueueHandle_t OS_dcMotorTaskQueue = NULL;
TaskHandle_t OS_tempSensTaskHandle = NULL;

static const char *TAG = "MAIN";

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

void app_main(void)
{
    ESP_LOGI(TAG, "System starting - Free heap: %lu bytes", esp_get_free_heap_size());
    
    OS_btnHndlrsTaskQueue = xQueueCreate(16, sizeof(ST_extEventMsg));
    configASSERT(OS_btnHndlrsTaskQueue != NULL);
    OS_bleEventQueue = xQueueCreate(16, sizeof(ST_bleMsg));
    configASSERT(OS_bleEventQueue != NULL);
    OS_dcMotorTaskQueue = xQueueCreate(2, sizeof(ST_dcMotorMsg));
    configASSERT(OS_dcMotorTaskQueue != NULL);

    nvs_init();
    
    // Initialize BLE first (PHY callbacks will manage SPI bus suspension/restoration)
    vTaskDelay(pdMS_TO_TICKS(100));
    bluetooth_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize SPI peripherals after BLE (SPI bus is now safe to use)
    spiConfig_configureSpiBus();
    display_lcdInit();
    display_uiInit();
    display_createMainMenu();
    display_createSelectRoastMenu();
    display_showMainMenu(NULL, 0);
    tempSens_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    tempSens_suspendTask();
    
    ESP_LOGI(TAG, "Stopping BLE advertising for triac init");
    bluetooth_stopAdvertising();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize remaining peripherals
    btnHndlrs_btnHndlrsInit();
    btn_configButtons();
    vTaskDelay(pdMS_TO_TICKS(100));
    triac_triacInit();
    pid_PIDInit();
    DCMotor_initDcMotors();
    DCMotor_initTask();
    DCMotor_setMiniFanState(true);
    
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Restarting BLE advertising");
    bluetooth_startAdvertising();

#if BUTTON_DEBUG
    OS_mainTaskQueue = xQueueCreate(32, sizeof(EN_buttons));
    configASSERT(OS_mainTaskQueue != NULL);
    xTaskCreatePinnedToCore(mainTask, "mainTask", 4096, NULL, configMAX_PRIORITIES - 1, NULL, 1);
#else
    xTaskCreatePinnedToCore(btnHndlrs_task, "btnHndlrsTask", 8192, NULL, configMAX_PRIORITIES - 1, NULL, 1);
    xTaskCreatePinnedToCore(bluetooth_task, "bluetoothTask", 8192, NULL, configMAX_PRIORITIES - 2, NULL, 1);
    xTaskCreatePinnedToCore(tempSens_task, "tempSensTask", 4096, NULL, configMAX_PRIORITIES - 3, &OS_tempSensTaskHandle, 1);
    xTaskCreatePinnedToCore(DCMotor_task, "dcMotorTask", 4096, NULL, configMAX_PRIORITIES - 4, NULL, 1);
#endif
}

#if BUTTON_DEBUG
static void mainTask(void *arg)
{
    float temp = 0.0f;
    float pwm = 0.0f;
    float brightness = 80.0f;
    EN_buttons msg;

    while (1) {
        if (xQueueReceive(OS_mainTaskQueue, &msg, pdMS_TO_TICKS(2000)) == pdTRUE)
        {
            switch (msg)
            {
            case BTN_1_PRESSED:
                ESP_LOGI(TAG, "Button 1 pressed");
                // float currTemp = tempSens_getTemperature();
                // float *tempProfile = calloc((MAX_ROAST_TIME_IN_MIN / 4), sizeof(float));
                // for (uint32_t i = 0; i < (MAX_ROAST_TIME_IN_MIN / 4); i++)
                // {
                //     tempProfile[i] = currTemp + random() % 20;
                // }

                // display_createRoastChart(tempProfile, (MAX_ROAST_TIME_IN_MIN / 4), NAN, NAN);
                // display_showRoastChart();
                // pid_ctrlLoopStart(tempProfile, (MAX_ROAST_TIME_IN_MIN / 4));
                // free(tempProfile);
                triac_setState(true);
                // DCMotor_turnFansOnForSeconds(100);
                break;
            case BTN_2_PRESSED:
                ESP_LOGI(TAG, "Button 2 pressed");
                ESP_LOGI(TAG, "Current pwm %.2f%%", pwm);
                triac_setPwrPercent(pwm);
                pwm += 1.0f;
                break;
            case BTN_3_PRESSED:
                ESP_LOGI(TAG, "Button 3 pressed");
                ESP_LOGI(TAG, "Current pwm %.2f%%", pwm);
                triac_setPwrPercent(pwm);
                (pwm >= 1.0) ? (pwm -= 1.0f) : (pwm = 0.0f);
                break;
            case BTN_4_PRESSED:
                ESP_LOGI(TAG, "Button 4 pressed");
                triac_setPwrPercent(100.0f);
                break;
            default:
                ESP_LOGW(TAG, "Unknown button press: %d", msg);
                break;
            }
        }
        else
        {
            // ESP_LOGI(TAG, "Current pwm %.2f%%", pwm);
            // temp = tempSens_getTemperature();
            // ESP_LOGI(TAG, "Current Temperature: %.2f°C", temp);
        }
        // the following logging format is according to the requirement of serial-studio frame format
        // also see the dashboard config file `serial-studio-dashboard.json` for more information
#if SERIAL_STUDIO_DEBUG
        printf("/*%d*/\r\n", motorCtrlCtx.report_pulses);
#endif
    }
}
#endif