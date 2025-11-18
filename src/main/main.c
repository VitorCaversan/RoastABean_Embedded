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

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

// Enable this config,  we will print debug formated string, which in return can be captured and parsed by Serial-Studio
#define SERIAL_STUDIO_DEBUG           CONFIG_SERIAL_STUDIO_DEBUG
#define BUTTON_DEBUG                 0

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

static void mainTask(void *arg);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

QueueHandle_t OS_mainTaskQueue = NULL;
QueueHandle_t OS_btnHndlrsTaskQueue = NULL;

static const char *TAG = "MAIN";

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

void app_main(void)
{
    OS_mainTaskQueue = xQueueCreate(32, sizeof(EN_buttons));
    configASSERT(OS_mainTaskQueue != NULL);
    OS_btnHndlrsTaskQueue = xQueueCreate(32, sizeof(ST_screenMsg));
    configASSERT(OS_btnHndlrsTaskQueue != NULL);

    spiConfig_configureSpiBus();
    display_lcdInit();
    display_uiInit();
    
    // Pre-create all menu btnHndlrs
    display_createMainMenu();
    
    // Create select roast menu (labels will be updated when shown)
    display_createSelectRoastMenu();
    
    // Show start menu initially
    display_showMainMenu();
    
    btnHndlrs_btnHndlrsInit();
    tempSens_init();
    btn_configButtons();
    triac_triacInit();
    pid_PIDInit();
    DCMotor_initDcMotors();

    xTaskCreatePinnedToCore(mainTask, "mainTask", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);
    xTaskCreatePinnedToCore(btnHndlrs_btnHndlrsTask, "btnHndlrsTask", 8192, NULL, configMAX_PRIORITIES - 3, NULL, 1);
}

static void mainTask(void *arg)
{
    ST_motorControlContext *motorCtrlCtx = DCMotor_getContextFromMotor(TB_BOARD_1);

    ESP_LOGI(TAG, "Enable motor forward");
    tb6612_setSpeed(&motorCtrlCtx->motor, MOTOR_A, 0);
    tb6612_setSpeed(&motorCtrlCtx->motor, MOTOR_B, 0);

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
                    btnHndlrs_onBtnPress(BTN_1_PRESSED);
                break;
                case BTN_2_PRESSED:
                    ESP_LOGI(TAG, "Button 2 pressed");
                    btnHndlrs_onBtnPress(BTN_2_PRESSED);
                break;
                case BTN_3_PRESSED:
                    ESP_LOGI(TAG, "Button 3 pressed");
                    btnHndlrs_onBtnPress(BTN_3_PRESSED);
                break;
                case BTN_4_PRESSED:
                    ESP_LOGI(TAG, "Button 4 pressed");
                    btnHndlrs_onBtnPress(BTN_4_PRESSED);
                break;
                default:
                    ESP_LOGW(TAG, "Unknown button press: %d", msg);
                break;
#if BUTTON_DEBUG
            case BTN_1_PRESSED:
                ESP_LOGI(TAG, "Button 1 pressed");
                float currTemp = tempSens_getTemperature();
                float *tempProfile = calloc((MAX_ROAST_TIME_IN_MIN / 4), sizeof(float));
                for (uint32_t i = 0; i < (MAX_ROAST_TIME_IN_MIN / 4); i++)
                {
                    tempProfile[i] = currTemp + random() % 20;
                }

                display_createRoastChart(tempProfile, (MAX_ROAST_TIME_IN_MIN / 4), NAN, NAN);
                display_showRoastChart();
                pid_ctrlLoopStart(tempProfile, (MAX_ROAST_TIME_IN_MIN / 4));
                free(tempProfile);
                break;
            case BTN_2_PRESSED:
                ESP_LOGI(TAG, "Button 2 pressed");
                ESP_LOGI(TAG, "Current pwm %.2f%%", pwm);
                triac_setPwrPercent(pwm);
                pwm += 1.0f;
                break;
            case BTN_3_PRESSED:
                ESP_LOGI(TAG, "Button 3 pressed");
                triac_setPwrPercent(50.0f);
                break;
            case BTN_4_PRESSED:
                ESP_LOGI(TAG, "Button 4 pressed");
                triac_setPwrPercent(100.0f);
                break;
            default:
                ESP_LOGW(TAG, "Unknown button press: %d", msg);
                break;
#endif
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