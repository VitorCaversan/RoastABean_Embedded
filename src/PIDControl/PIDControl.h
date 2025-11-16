#ifndef PID_CTRL_H
#define PID_CTRL_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "pid_ctrl.h"

#include "OSConfig.h"
#include "TriacControl.h"
#include "TempSens.h"
#include "Display.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define US_TO_SECONDS(us)          ((us) / 1000000ULL)

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef struct ST_pidCtrlContext
{
    pid_ctrl_block_handle_t pidCtrl;
    unsigned long startingProcessUs;
    float tempProfile[MAX_ROAST_TIME_IN_MIN];
    unsigned long minsToControl;
} ST_pidCtrlContext;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Configure PID control with TempSens as input and TriacControl as output
 */
extern void pid_PIDInit(void);

/**
 * @brief Configure and start the PID control loop
 * 
 * @param tempProfile Array with the target temperature profile in °C/min
 * @param minsToControl Duration to control the roasting process in minutes
 */
extern void pid_ctrlLoopStart(float *tempProfile, uint32_t minsToControl);
/**
 * @brief Stop the PID control loop
 */
extern void pid_ctrlLoopStop(void);

#endif // PID_CTRL_H