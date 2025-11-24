#ifndef TRIAC_CTRL_H
#define TRIAC_CTRL_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp32-triac-dimmer-driver.h"

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
 * @brief Sets the triac ON or OFF
 * 
 * @param onOff true = ON, false = OFF
 */
extern void triac_setState(bool onOff);

/**
 * @brief Sets the duty cycle
 * 
 * @param pwm From 0 to 100
 */
extern void triac_setPwrPercent(float pwm);


#endif // TRIAC_CTRL_H