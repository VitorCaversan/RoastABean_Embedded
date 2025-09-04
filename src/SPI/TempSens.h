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

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

extern void TempSens_init(void);

extern float TempSens_getTemperature(void);

extern float TempSens_getColdJunctionTemperature(void);

#endif // TEMP_SENS_H