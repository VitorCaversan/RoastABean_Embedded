#ifndef TRIAC_CTRL_H
#define TRIAC_CTRL_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Configure Triac module for control using a PWM output
 */
extern void triac_triacInit(void);

/**
 * @brief Sets the duty cycle
 * 
 * @param pwm From 0 to 100
 */
extern void triac_setPwmPercent(float pwm);

/**
 * @brief Stops the PWM output channel
 */
extern void triac_stopChannel(void);

#endif // TRIAC_CTRL_H