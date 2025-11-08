/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "Screens.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Update the roast chart with new data.
 *
 * @param data Pointer to the chart update data.
 */
static void updateChart(const ST_chartUpdateData *data);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "SCREENS";

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void scr_screensTask(void *arg)
{
    ST_screenMsg msg;

    while (1)
    {
        if (xQueueReceive(OS_screensTaskQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg.event)
            {
            case SCR_EVENT_UPDATE:
                ESP_LOGI(TAG, "Update screen");
                break;
            case SCR_EVENT_SHOW_CHART:
                ESP_LOGI(TAG, "Show chart");
                break;
            case SCR_EVENT_HIDE_CHART:
                ESP_LOGI(TAG, "Hide chart");
                break;
            case SCR_EVENT_UPDATE_CHART:
                ESP_LOGI(TAG, "Update chart");
                updateChart((ST_chartUpdateData *)msg.data);
                break;
            default:
                ESP_LOGW(TAG, "Unknown screen event: %d", msg.event);
                break;
            }
        }
    }
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void updateChart(const ST_chartUpdateData *data)
{
    display_updateRoastChart(data->currTargetTemp, data->elapsedSecs, data->currentTemp);
}