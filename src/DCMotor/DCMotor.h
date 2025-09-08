#ifndef DC_MOTOR_H
#define DC_MOTOR_H

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
#include "driver/pulse_cnt.h"
#include "bdc_motor.h"
#include "pid_ctrl.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define DCMOTOR_PID_CTRL_ENABLED 0

/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/
typedef struct {
    bdc_motor_handle_t motor;
#if DCMOTOR_PID_CTRL_ENABLED
    pcnt_unit_handle_t pcnt_encoder;
    pid_ctrl_block_handle_t pid_ctrl;
    int report_pulses;
#endif
} motor_control_context_t;

/*******************************************************************************
 * EXTERNAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initialize the DC motors using bdc_motor library
 */
extern void DCMotor_initDcMotors(void);

#if DCMOTOR_PID_CTRL_ENABLED
extern void DCMotor_initPulseCntrs(void);

extern void DCMotor_initPIDCtrl(void);
#endif

extern motor_control_context_t *DCMotor_getContext(void);

#endif // DC_MOTOR_H