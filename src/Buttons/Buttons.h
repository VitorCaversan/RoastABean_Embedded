#ifndef BUTTONS_H
#define BUTTONS_H

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "OSConfig.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

typedef enum EN_buttons
{
    BTN_1_PRESSED = 0,
    BTN_2_PRESSED,
    BTN_3_PRESSED,
    BTN_4_PRESSED,

    BTN_QTY // Must be the last element
} EN_buttons;

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Configure buttons GPIOs and interrupts
 * 
 * When a button is pressed, an event is sent to the main task queue
 */
extern void btn_configButtons(void);

#endif // BUTTONS_H