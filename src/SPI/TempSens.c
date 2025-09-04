/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "TempSens.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

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

void TempSens_init(void)
{
    ESP_LOGI(TAG, "Init MAX31855");
    max31855_init_desc(&max31855Module, HELPER_SPI_HOST_DEFAULT, MAX31855_MAX_CLOCK_SPEED_HZ, GPIO_NUM_2);
}

float TempSens_getTemperature(void)
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
        ESP_LOGI(TAG, "Temperature: %.2f°C", tempInCelcius);
        return tempInCelcius;
    }

    return -1.0f;
}

float TempSens_getColdJunctionTemperature(void)
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