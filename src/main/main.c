/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

static const char *TAG = "MAIN";

// Enable this config,  we will print debug formated string, which in return can be captured and parsed by Serial-Studio
#define SERIAL_STUDIO_DEBUG           CONFIG_SERIAL_STUDIO_DEBUG


void app_main(void)
{
    spiConfig_configureSpiBus();
    display_lcdInit();
    display_uiInit();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    tempSens_init();
    DCMotor_initDcMotors();

    ST_motorControlContext *motor_ctrl_ctx = DCMotor_getContextFromMotor(MOTOR_1);

    ESP_LOGI(TAG, "Enable motor forward");
    bdc_motor_enable(motor_ctrl_ctx->motor);
    bdc_motor_forward(motor_ctrl_ctx->motor);

    float temp = 0.0f;

    while (1) {
        temp = tempSens_getTemperature();

        if (temp < 25.0f)
        {
            ESP_LOGI(TAG, "Temperature is below 25°C");
            bdc_motor_set_speed(motor_ctrl_ctx->motor, 25);
        }
        else if (temp < 30.0f)
        {
            ESP_LOGI(TAG, "Temperature is below 30°C");
            bdc_motor_set_speed(motor_ctrl_ctx->motor, 50);
        }
        else
        {
            ESP_LOGI(TAG, "Temperature is above 30°C");
            bdc_motor_set_speed(motor_ctrl_ctx->motor, 75);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        // the following logging format is according to the requirement of serial-studio frame format
        // also see the dashboard config file `serial-studio-dashboard.json` for more information
#if SERIAL_STUDIO_DEBUG
        printf("/*%d*/\r\n", motor_ctrl_ctx.report_pulses);
#endif
    }
}
