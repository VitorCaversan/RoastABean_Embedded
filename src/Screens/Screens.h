#ifndef SCREENS_H
#define SCREENS_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "OSConfig.h"
#include "Display.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

typedef enum EN_screenEvents
{
    SCR_EVENT_NONE = 0,
    SCR_EVENT_UPDATE,
    SCR_EVENT_SHOW_CHART,
    SCR_EVENT_HIDE_CHART,
    SCR_EVENT_UPDATE_CHART,
    
    SCR_EVENT_QTY // Must be the last element
} EN_screenEvents;

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef struct ST_screenMsg
{
    EN_screenEvents event;
    void *data;
} ST_screenMsg;

typedef struct ST_chartUpdateData
{
    float currTargetTemp;
    uint32_t elapsedSecs;
    float currentTemp;
} ST_chartUpdateData;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Task to handle screens
 */
extern void scr_screensTask(void *arg);

#endif // SCREENS_H
