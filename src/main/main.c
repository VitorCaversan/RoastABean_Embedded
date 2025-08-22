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
#include <max31855.h>
#include <esp_idf_lib_helpers.h>
#include "DCMotor.h"

static const char *TAG = "MAIN";

// Enable this config,  we will print debug formated string, which in return can be captured and parsed by Serial-Studio
#define SERIAL_STUDIO_DEBUG           CONFIG_SERIAL_STUDIO_DEBUG


void app_main(void)
{
    DCMotor_initDcMotors();

    motor_control_context_t *motor_ctrl_ctx = DCMotor_getContext();

    ESP_LOGI(TAG, "Enable motor forward");
    bdc_motor_enable(motor_ctrl_ctx->motor);
    bdc_motor_forward(motor_ctrl_ctx->motor);

    // MAX31855 CONFIG
    max31855_t dev = { 0 };
    // Configure SPI bus
    spi_bus_config_t cfg =
    {
        .mosi_io_num = -1,
        .miso_io_num = GPIO_NUM_15,
        .sclk_io_num = GPIO_NUM_1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HELPER_SPI_HOST_DEFAULT, &cfg, 1));

    // Init device
    ESP_ERROR_CHECK(max31855_init_desc(&dev, HELPER_SPI_HOST_DEFAULT, MAX31855_MAX_CLOCK_SPEED_HZ, GPIO_NUM_2));

    float tc_t, cj_t;
    bool scv, scg, oc;

    while (1) {
        esp_err_t res = max31855_get_temperature(&dev, &tc_t, &cj_t, &scv, &scg, &oc);
        if (res != ESP_OK)
            ESP_LOGE(TAG, "Failed to measure: %d (%s)", res, esp_err_to_name(res));
        else
        {
            if (scv) ESP_LOGW(TAG, "Thermocouple shorted to VCC!");
            if (scg) ESP_LOGW(TAG, "Thermocouple shorted to GND!");
            if (oc) ESP_LOGW(TAG, "No connection to thermocouple!");
            ESP_LOGI(TAG, "Temperature: %.2f°C, cold junction temperature: %.4f°C", tc_t, cj_t);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        // the following logging format is according to the requirement of serial-studio frame format
        // also see the dashboard config file `serial-studio-dashboard.json` for more information
#if SERIAL_STUDIO_DEBUG
        printf("/*%d*/\r\n", motor_ctrl_ctx.report_pulses);
#endif
    }
}
