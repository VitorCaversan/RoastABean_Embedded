#ifndef TEMP_SENS_H
#define TEMP_SENS_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <max31855.h>
#include <esp_idf_lib_helpers.h>

#include "SpiConfig.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define TEMP_SENS_INVALID_TEMPERATURE -300.0f

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initialize the MAX31855 thermocouple-to-digital converter library
 */
extern void tempSens_init(void);

/**
 * @brief Get the temperature reading from the MAX31855 in Celsius
 * 
 * @return float Temperature in Celsius, or -300.0f if an error occurred
 */
extern float tempSens_getTemperature(void);

extern float tempSens_getColdJunctionTemperature(void);

#endif // TEMP_SENS_H