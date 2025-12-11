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
#include "pid_ctrl.h"
#include "tb6612.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define DCMOTOR_PID_CTRL_ENABLED 0

#define BDC_MOTOR_MAX_SPEED             400
/*******************************************************************************
 * TYPEDEFS AND STRUCTURES
 ******************************************************************************/

typedef enum EN_TbBoard
{
    TB_BOARD_1 = 0,
    TB_BOARD_2,

    MOTORS_QTY // Must be the last element
} EN_TbBoard;

typedef struct {
    tb6612Handle_t motor;
#if DCMOTOR_PID_CTRL_ENABLED
    pcnt_unit_handle_t pcnt_encoder;
    pid_ctrl_block_handle_t pid_ctrl;
    int report_pulses;
#endif
} ST_motorControlContext;

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

/**
 * @brief Get the motor control context for a specific motor
 * 
 * TB_BOARD_1 -> Motor 1 and 2
 * TB_BOARD_2 -> Motor 2 and 3
 * 
 * @param motor The desired board
 * @return ST_motorControlContext*
 */
extern ST_motorControlContext *DCMotor_getContextFromMotor(EN_TbBoard motor);

/**
 * @brief Speed motor up in intervals of 1 second
 * 
 * @param tbBoard The TB board where the motor is connected
 * @param motorId The motor id
 * @param startingPercent Starting point for the ramp
 * @param targetPercent Endpoint for the ramp
 * @param step The steps taken every second
 */
extern void DCMotor_rampSpeedUp(EN_TbBoard tbBoard,
                                EN_tbMotorId motorId,
                                float startingPercent,
                                float targetPercent,
                                float step);

/**
 * @brief Turns the fans on and sets a timer to turn them off after specified seconds
 * 
 * @param seconds Duration in seconds to keep the fans on
 */
extern void DCMotor_turnFansOnForSeconds(uint32_t seconds);

#endif // DC_MOTOR_H