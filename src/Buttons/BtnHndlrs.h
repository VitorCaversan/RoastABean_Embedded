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
#include "Buttons.h"
#include "TempSens.h"
#include "PIDControl.h"
#include "DCMotor.h"
#include "Bluetooth.h"
#include "NVShndlr.h"
#include "JsonHndlr.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define MAX_CHARTS_TO_SHOW      3

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef struct ST_btnsFunc
{
    void (*onBtn1Pressed)(void);
    void (*onBtn2Pressed)(void);
    void (*onBtn3Pressed)(void);
    void (*onBtn4Pressed)(void);
} ST_btnsFunc;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Task to handle btnHndlrs
 */
extern void btnHndlrs_task(void *arg);

/**
 * @brief Initialize the btnHndlrs module by setting up button callbacks
 */
extern void btnHndlrs_btnHndlrsInit(void);

#endif // SCREENS_H
