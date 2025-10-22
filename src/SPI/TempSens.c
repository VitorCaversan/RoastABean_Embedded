/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "TempSens.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define TEMP_SENS_CS_PIN           GPIO_NUM_1

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static max31855_t max31855Module = {0};
static const char *TAG = "TEMP_SENS";

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

void tempSens_init(void)
{
    ESP_LOGI(TAG, "Init MAX31855");
    max31855_init_desc(&max31855Module, DEFAULT_SPI_HOST, MAX31855_MAX_CLOCK_SPEED_HZ, TEMP_SENS_CS_PIN);
}

float tempSens_getTemperature(void)
{
    float tempInCelcius, coldJunctionTemp;
    bool scv, scg, oc;

    esp_err_t res = max31855_get_temperature(&max31855Module, &tempInCelcius, &coldJunctionTemp, &scv, &scg, &oc);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to measure: %d (%s)", res, esp_err_to_name(res));
        return TEMP_SENS_INVALID_TEMPERATURE;
    }
    else
    {
        if (scv) ESP_LOGW(TAG, "Thermocouple shorted to VCC!");
        if (scg) ESP_LOGW(TAG, "Thermocouple shorted to GND!");
        if (oc) ESP_LOGW(TAG, "No connection to thermocouple!");
        ESP_LOGI(TAG, "Temperature: %.2f°C", tempInCelcius);
        return tempInCelcius;
    }
}

float tempSens_getColdJunctionTemperature(void)
{
    float tempInCelcius, coldJunctionTemp;
    bool scv, scg, oc;

    esp_err_t res = max31855_get_temperature(&max31855Module, &tempInCelcius, &coldJunctionTemp, &scv, &scg, &oc);
    if (res != ESP_OK)
        ESP_LOGE(TAG, "Failed to measure: %d (%s)", res, esp_err_to_name(res));
    else
    {
        if (scv) ESP_LOGW(TAG, "Thermocouple shorted to VCC!");
        if (scg) ESP_LOGW(TAG, "Thermocouple shorted to GND!");
        if (oc) ESP_LOGW(TAG, "No connection to thermocouple!");
        ESP_LOGI(TAG, "Cold junction temperature: %.2f°C", coldJunctionTemp);
        return coldJunctionTemp;
    }

    return -1.0f;
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/