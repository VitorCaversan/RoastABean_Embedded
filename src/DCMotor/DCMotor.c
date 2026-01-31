/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "DCMotor.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/
#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000 // 10MHz, 1 tick = 0.1us
#define BDC_MCPWM_FREQ_HZ             25000    // 25KHz PWM
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ) // maximum value we can set for the duty cycle, in ticks

#define MOTOR1_IN_A              GPIO_NUM_38
#define MOTOR1_IN_B              GPIO_NUM_37
#define MOTOR2_IN_A              GPIO_NUM_39
#define MOTOR2_IN_B              40
#define MOTOR3_IN_A              -1
#define MOTOR3_IN_B              -1
#define MOTOR4_IN_A              -1
#define MOTOR4_IN_B              -1

#define MOTOR1_PWM_PIN              36
#define MOTOR2_PWM_PIN              41
#define MOTOR3_PWM_PIN              42
#define MOTOR4_PWM_PIN              48

#define MOTOR_STBY_PIN              47

#define BDC_ENCODER_GPIO_A            -1
#define BDC_ENCODER_GPIO_B            -1
#define BDC_ENCODER_PCNT_HIGH_LIMIT   1000
#define BDC_ENCODER_PCNT_LOW_LIMIT    -1000

#define BDC_PID_LOOP_PERIOD_MS        10   // calculate the motor speed every 10ms

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/
#if DCMOTOR_PID_CTRL_ENABLED
static void pid_loop_cb(void *args);
#endif

/**
 * @brief Callback to turn off the fans after a timeout
 * 
 * @param arg
 */
static void turnFansOffCallback(void *arg);

/**
 * @brief Speed motor up in intervals of 200ms
 * 
 * @param tbBoard The TB board where the motor is connected
 * @param motorId The motor id
 * @param targetPercent Endpoint for the ramp
 * @param step The steps taken every 200ms
 */
static void rampSpeed(EN_TbBoard tbBoard,
                      EN_tbMotorId motorId,
                      float targetPercent,
                      float step);

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/
static ST_motorControlContext motorsCtrlCntxt[MOTORS_QTY] = {0};
static const char *TAG = "DCMOTOR";

/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void DCMotor_task(void *arg)
{
    ST_dcMotorMsg msg;
    
    while (1)
    {
        if (xQueueReceive(OS_dcMotorTaskQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg.event)
            {
                case DCMOTOR_EVENT_RAMP_MOTOR_SPEED:
                    if (msg.data != NULL)
                    {
                        ST_rampSpeedData *rampData = (ST_rampSpeedData *)msg.data;
                        
                        ESP_LOGI(TAG, "Ramping motor speed - Board: %d, Motor: %d, Target: %.2f%%, Step: %.2f%%",
                                rampData->tbBoard, rampData->motorId,
                                rampData->targetPercent, rampData->step);
                        
                        rampSpeed(rampData->tbBoard,
                                    rampData->motorId,
                                    rampData->targetPercent,
                                    rampData->step);
                        
                        free(msg.data);
                    }
                break;
                
                default:
                    ESP_LOGW(TAG, "Unknown DC motor event: %d", msg.event);
                    if (msg.data != NULL)
                    {
                        free(msg.data);
                    }
                break;
            }
        }
    }
}

extern void DCMotor_initTask(void)
{
    OS_dcMotorTaskQueue = xQueueCreate(16, sizeof(ST_dcMotorMsg));
    configASSERT(OS_dcMotorTaskQueue != NULL);
    
    ESP_LOGI(TAG, "DC Motor task initialized");
}

extern void DCMotor_initDcMotors(void)
{
    tb6612Config_t cfg = {
        .stbyGpio   = MOTOR_STBY_PIN,      // STBY pin (tie HIGH if not using)
        .ain1Gpio   = MOTOR1_IN_A, .ain2Gpio = MOTOR1_IN_B, .pwmaGpio = MOTOR1_PWM_PIN,
        .bin1Gpio   = MOTOR2_IN_A, .bin2Gpio = MOTOR2_IN_B, .pwmbGpio = MOTOR2_PWM_PIN,
        .ledcMode   = LEDC_LOW_SPEED_MODE,
        .ledcTimer  = LEDC_TIMER_2,
        .pwmFreqHz  = 5000,
        .dutyRes    = LEDC_TIMER_10_BIT,
        .chA        = LEDC_CHANNEL_2,
        .chB        = LEDC_CHANNEL_3,
    };
    ESP_ERROR_CHECK(tb6612_init(&motorsCtrlCntxt[TB_BOARD_1].motor, &cfg));
    ESP_LOGI(TAG, "Created TB board 1");

    tb6612Config_t cfg2 = {
        .stbyGpio   = MOTOR_STBY_PIN,      // STBY pin (tie HIGH if not using)
        .ain1Gpio   = MOTOR3_IN_A, .ain2Gpio = MOTOR3_IN_B, .pwmaGpio = MOTOR3_PWM_PIN,
        .bin1Gpio   = MOTOR4_IN_A, .bin2Gpio = MOTOR4_IN_B, .pwmbGpio = MOTOR4_PWM_PIN,
        .ledcMode   = LEDC_LOW_SPEED_MODE,
        .ledcTimer  = LEDC_TIMER_3,
        .pwmFreqHz  = 5000,
        .dutyRes    = LEDC_TIMER_10_BIT,
        .chA        = LEDC_CHANNEL_4,
        .chB        = LEDC_CHANNEL_5,
    };
    ESP_ERROR_CHECK(tb6612_init(&motorsCtrlCntxt[TB_BOARD_2].motor, &cfg2));
    ESP_LOGI(TAG, "Created TB board 2");

    return;
}

#if DCMOTOR_PID_CTRL_ENABLED
extern void DCMotor_initPulseCntrs(void)
{
    ESP_LOGI(TAG, "Init pcnt driver to decode rotary signal");
    pcnt_unit_config_t unit_config = {
        .high_limit = BDC_ENCODER_PCNT_HIGH_LIMIT,
        .low_limit = BDC_ENCODER_PCNT_LOW_LIMIT,
        .flags.accum_count = true, // enable counter accumulation
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = BDC_ENCODER_GPIO_A,
        .level_gpio_num = BDC_ENCODER_GPIO_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = BDC_ENCODER_GPIO_B,
        .level_gpio_num = BDC_ENCODER_GPIO_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, BDC_ENCODER_PCNT_HIGH_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, BDC_ENCODER_PCNT_LOW_LIMIT));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    
    motorsCtrlCntxt.pcnt_encoder = pcnt_unit;
}

extern void DCMotor_initPIDCtrl(void)
{
    ESP_LOGI(TAG, "Create PID control block");
    pid_ctrl_parameter_t pid_runtime_param = {
        .kp = 0.6,
        .ki = 0.4,
        .kd = 0.2,
        .cal_type = PID_CAL_TYPE_INCREMENTAL,
        .max_output   = BDC_MCPWM_DUTY_TICK_MAX - 1,
        .min_output   = 0,
        .max_integral = 1000,
        .min_integral = -1000,
    };
    pid_ctrl_block_handle_t pid_ctrl = NULL;
    pid_ctrl_config_t pid_config = {
        .init_param = pid_runtime_param,
    };
    ESP_ERROR_CHECK(pid_new_control_block(&pid_config, &pid_ctrl));

    motorsCtrlCntxt.pid_ctrl = pid_ctrl;

    ESP_LOGI(TAG, "Create a timer to do PID calculation periodically");
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = pid_loop_cb,
        .arg = &motorCtrlCtx,
        .name = "pid_loop"
    };
    esp_timer_handle_t pid_loop_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pid_loop_timer));

    ESP_LOGI(TAG, "Enable motor");
    ESP_ERROR_CHECK(bdc_motor_enable(motor));
    ESP_LOGI(TAG, "Forward motor");
    ESP_ERROR_CHECK(bdc_motor_forward(motor));

    ESP_LOGI(TAG, "Start motor speed loop");
    ESP_ERROR_CHECK(esp_timer_start_periodic(pid_loop_timer, BDC_PID_LOOP_PERIOD_MS * 1000));

}
#endif

extern ST_motorControlContext *DCMotor_getContextFromMotor(EN_TbBoard motor)
{
    return &motorsCtrlCntxt[motor];
}

extern void DCMotor_rampSpeed(EN_TbBoard tbBoard,
                              EN_tbMotorId motorId,
                              float targetPercent,
                              float step,
                              bool isFromISR)
{
    ST_rampSpeedData *rampData = malloc(sizeof(ST_rampSpeedData));
    if (rampData == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for ramp speed data");
        return;
    }

    rampData->tbBoard = tbBoard;
    rampData->motorId = motorId;
    rampData->targetPercent = targetPercent;
    rampData->step = step;

    ST_dcMotorMsg msg = {
        .event = DCMOTOR_EVENT_RAMP_MOTOR_SPEED,
        .data = (void *)rampData
    };

    if (isFromISR)
    {
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        if (xQueueSendFromISR(OS_dcMotorTaskQueue, &msg, &higherPriorityTaskWoken) != pdTRUE)
        {
            ESP_LOGE(TAG, "Failed to send ramp speed message from ISR");
            free(rampData);
        }

        if (higherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
    else
    {
        if (xQueueSend(OS_dcMotorTaskQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            ESP_LOGE(TAG, "Failed to send ramp speed message");
            free(rampData);
        }
    }
}

extern void DCMotor_turnFansOnForSeconds(uint32_t seconds)
{
    ESP_LOGI(TAG, "Turning fans on for %u seconds", seconds);

    ST_motorControlContext *motorCtx = DCMotor_getContextFromMotor(TB_BOARD_1);

    tb6612_setSpeed(&motorCtx->motor, MOTOR_B, -100.0);

    const esp_timer_create_args_t oneshotTimerArgs = {
        .callback = turnFansOffCallback,
        .arg = NULL,
        .name = "turn_fans_off"
    };
    esp_timer_handle_t oneshotTimer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&oneshotTimerArgs, &oneshotTimer));
    ESP_ERROR_CHECK(esp_timer_start_once(oneshotTimer, seconds * 1000000));
}

extern void DCMotor_setMiniFanState(bool on)
{
    ESP_LOGI(TAG, "Setting mini fan state to: %s", on ? "ON" : "OFF");

    ST_motorControlContext *motorCtx = DCMotor_getContextFromMotor(TB_BOARD_2);

    if (on)
    {
        tb6612_setSpeed(&motorCtx->motor, MOTOR_B, 100.0);
    }
    else
    {
        tb6612_setSpeed(&motorCtx->motor, MOTOR_B, 0);
    }
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

#if DCMOTOR_PID_CTRL_ENABLED
static void pid_loop_cb(void *args)
{
    static int last_pulse_count = 0;
    ST_motorControlContext *ctx = (ST_motorControlContext *)args;
    pcnt_unit_handle_t pcnt_unit = ctx->pcnt_encoder;
    pid_ctrl_block_handle_t pid_ctrl = ctx->pid_ctrl;
    bdc_motor_handle_t motor = ctx->motor;

    // get the result from rotary encoder
    int cur_pulse_count = 0;
    pcnt_unit_get_count(pcnt_unit, &cur_pulse_count);
    int real_pulses = cur_pulse_count - last_pulse_count;
    last_pulse_count = cur_pulse_count;
    ctx->report_pulses = real_pulses;

    // calculate the speed error
    float error = BDC_MOTOR_MAX_SPEED - real_pulses;
    float new_speed = 0;

    // set the new speed
    pid_compute(pid_ctrl, error, &new_speed);
    bdc_motor_set_speed(motor, (uint32_t)new_speed);
}
#endif

static void turnFansOffCallback(void *arg)
{
    ESP_LOGI(TAG, "Turning fans off after timeout");
    ST_motorControlContext *motorCtx = DCMotor_getContextFromMotor(TB_BOARD_1);

    tb6612_setSpeed(&motorCtx->motor, MOTOR_B, 0);

    esp_timer_handle_t* timer_ptr = (esp_timer_handle_t*)arg;
    if (timer_ptr && *timer_ptr)
    {
        esp_timer_delete(*timer_ptr);
        *timer_ptr = NULL;
    }
}

static void rampSpeed(EN_TbBoard tbBoard,
                      EN_tbMotorId motorId,
                      float targetPercent,
                      float step)
{
    static float prevEndPercent = 0.0f;
    
    if ((step > 100.0f) || (step <= 0.0f))
    {
        ESP_LOGW(TAG, "Speed ramp not possible");
        return;
    }

    ST_motorControlContext *motorCtx = DCMotor_getContextFromMotor(tbBoard);

    if (prevEndPercent > targetPercent)
    {
        for (; prevEndPercent >= targetPercent; prevEndPercent -= step)
        {
            tb6612_setSpeed(&motorCtx->motor, motorId, prevEndPercent);
        
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    else
    {
        for (; prevEndPercent <= targetPercent; prevEndPercent += step)
        {
            tb6612_setSpeed(&motorCtx->motor, motorId, prevEndPercent);
        
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    tb6612_setSpeed(&motorCtx->motor, motorId, targetPercent);
    prevEndPercent = targetPercent;
}