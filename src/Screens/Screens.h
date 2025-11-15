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
 * @brief Task to handle screens
 */
extern void scr_screensTask(void *arg);

/**
 * @brief Initialize the screens module by setting up button callbacks
 */
extern void scr_screensInit(void);

/**
 * @brief Calls respective button press handler based on button pressed
 * 
 * @param button The button that was pressed
 */
extern void scr_onBtnPress(EN_buttons button);

#endif // SCREENS_H
