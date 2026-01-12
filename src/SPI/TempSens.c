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
static float currTempCelcius = TEMP_SENS_INVALID_TEMPERATURE;
static float currColdJunctionTemp = TEMP_SENS_INVALID_TEMPERATURE;
static const char *TAG = "TEMP_SENS";

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

void tempSens_init(void)
{
    ESP_LOGI(TAG, "Init MAX31855");
    max31855_init_desc(&max31855Module, DEFAULT_SPI_HOST, MAX31855_MAX_CLOCK_SPEED_HZ, TEMP_SENS_CS_PIN);
}

extern void tempSens_task(void *arg)
{
    while (1)
    {
        float tempInCelcius, coldJunctionTemp;
        bool scv, scg, oc;
    
        esp_err_t res = max31855_get_temperature(&max31855Module, &tempInCelcius, &coldJunctionTemp, &scv, &scg, &oc);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to measure: %d (%s)", res, esp_err_to_name(res));
        }
        else
        {
            if (scv) ESP_LOGW(TAG, "Thermocouple shorted to VCC!");
            if (scg) ESP_LOGW(TAG, "Thermocouple shorted to GND!");
            if (oc) ESP_LOGW(TAG, "No connection to thermocouple!");
            if ((fabs(tempInCelcius - currTempCelcius) > 0.25f) ||
                (fabs(coldJunctionTemp - currColdJunctionTemp) > 0.25f))
            {
                // ESP_LOGI(TAG, "Temperature: %.2f°C, Cold junction temperature: %.2f°C", tempInCelcius, coldJunctionTemp);
            }
            
            currTempCelcius = tempInCelcius;
            currColdJunctionTemp = coldJunctionTemp;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

extern void tempSens_suspendTask(void)
{
    if (OS_tempSensTaskHandle != NULL)
    {
        ESP_LOGI(TAG, "Suspending temp sensor task for popup creation");
        vTaskSuspend(OS_tempSensTaskHandle);
    }
}
extern void tempSens_resumeTask(void)
{
    if (OS_tempSensTaskHandle != NULL)
    {
        ESP_LOGI(TAG, "Resuming temp sensor task");
        vTaskResume(OS_tempSensTaskHandle);
    }
}

float tempSens_getTemperature(void)
{
    return currTempCelcius;
}

float tempSens_getColdJunctionTemperature(void)
{
    return currColdJunctionTemp;
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/