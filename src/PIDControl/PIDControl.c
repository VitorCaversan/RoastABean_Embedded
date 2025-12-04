/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "PIDControl.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define USECONDS_IN_1_MIN           60000000
#define BDC_PID_LOOP_PERIOD_US      200000

#define V_NOMINAL                   127.0f
#define K_FF                        10.0f // % per unit of (airFlowRatio - 1)
#define POW_FF                      1.0f

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/**
 * @brief Callback for the pid loop timer
 * 
 * @param args pidRoastCtrlBlock is passed as an argument
 */
static void pidLoopCallback(void *args);

/**
 * @brief Get the Target Temperature from the temperature profile based on minutes since start
 * and interpolating linearly if necessary
 * 
 * @param ctx Context containing the temperature profile
 * @param usSinceStart Microseconds since the start of the roasting process
 * @return float Target temperature in °C
 */
static float getTargetTemperature(ST_pidCtrlContext *ctx, unsigned long usSinceStart);

/**
 * @brief Calculate the blower feedforward value based on the input voltage
 * 
 * Function can be used in case the control loop needs to adjust for varying air
 * flow due to voltage changes
 * 
 * @param voltage Current voltage in Volts
 * @return float Blower feedforward value in %
 */
static inline float blowerFeedforwardFromVoltage(float voltage)
{
    float airFlowRatio = pow(voltage / V_NOMINAL, POW_FF);
    return K_FF * (airFlowRatio - 1.0f);
}

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

ST_pidCtrlContext pidRoastCtrlBlock = {0};
esp_timer_handle_t pidLoopTimer = NULL;
ST_chartUpdateData chartUpdateData = {0};

static const char *TAG = "PID_CTRL";


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void pid_PIDInit(void)
{
    ESP_LOGI(TAG, "Create PID control block");
    pid_ctrl_parameter_t pid_runtime_param = {
        .kp = 0.6,
        .ki = 0.4,
        .kd = 0.0,
        .max_output   = 99.0,
        .min_output   = 7.0,
        .max_integral = 100.0, // Anti-windup: 80% -> +-40/0.4 ~= +-100
        .min_integral = -100.0,
        .cal_type = PID_CAL_TYPE_POSITIONAL,
    };
    pid_ctrl_config_t pid_config = {
        .init_param = pid_runtime_param,
    };
    ESP_ERROR_CHECK(pid_new_control_block(&pid_config, &pidRoastCtrlBlock.pidCtrl));
}

extern void pid_ctrlLoopStart(float *tempProfile, uint32_t minsToControl)
{
    pidRoastCtrlBlock.startingProcessUs = esp_timer_get_time();
    pidRoastCtrlBlock.minsToControl = minsToControl;
    memcpy(pidRoastCtrlBlock.tempProfile, tempProfile, minsToControl * sizeof(float));

    ESP_LOGI(TAG, "Create a timer to do PID calculation periodically");
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = pidLoopCallback,
        .arg = &pidRoastCtrlBlock,
        .name = "pid_loop"
    };
    pidLoopTimer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &pidLoopTimer));

    ESP_LOGI(TAG, "Set triac to ON state");
    triac_setState(true);

    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Start PID control loop");
    ESP_ERROR_CHECK(esp_timer_start_periodic(pidLoopTimer, BDC_PID_LOOP_PERIOD_US));
}

extern void pid_ctrlLoopStop(void)
{
    ESP_LOGI(TAG, "Stop PID control loop");
    esp_timer_stop(pidLoopTimer);
    esp_timer_delete(pidLoopTimer);
    pidLoopTimer = NULL;

    ESP_LOGI(TAG, "Set triac to OFF state");
    triac_setState(false);
}

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void pidLoopCallback(void *args)
{
    ST_pidCtrlContext *ctx = (ST_pidCtrlContext *)args;
    pid_ctrl_block_handle_t pid_ctrl = ctx->pidCtrl;

    float currTemp = tempSens_getTemperature();
    if (currTemp == TEMP_SENS_INVALID_TEMPERATURE)
    {
        ESP_LOGI(TAG, "Invalid temperature reading, skipping PID calculation");
        return;
    }

    unsigned long nowUs = esp_timer_get_time();
    unsigned long elapsedUs = (nowUs - ctx->startingProcessUs);
    float targetTemp = getTargetTemperature(ctx, elapsedUs);
    float error = targetTemp - currTemp;
    
    float newPwrPercent = 0.0f;
    pid_compute(pid_ctrl, error, &newPwrPercent);
    if (newPwrPercent < 0.0f)
        newPwrPercent = 0.0f;
    if (newPwrPercent > 100.0f)
        newPwrPercent = 100.0f;
    triac_setPwrPercent(newPwrPercent);

    ST_extEventMsg screenMsg = {0};

    unsigned long elapsedMins = (elapsedUs / USECONDS_IN_1_MIN);
    if (elapsedMins >= ctx->minsToControl)
    {
        screenMsg.event = EXT_EVENT_END_ROAST;
        screenMsg.data = NULL;
    }
    else
    {
        chartUpdateData.currTargetTemp = targetTemp;
        chartUpdateData.elapsedSecs = US_TO_SECONDS(nowUs - ctx->startingProcessUs);
        chartUpdateData.currentTemp = currTemp;
    
        screenMsg.event = EXT_EVENT_UPDATE_CHART;
        screenMsg.data = &chartUpdateData;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(OS_btnHndlrsTaskQueue, &screenMsg, &xHigherPriorityTaskWoken);
    
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

static float getTargetTemperature(ST_pidCtrlContext *ctx, unsigned long usSinceStart)
{
    unsigned long minsSinceStart = usSinceStart / USECONDS_IN_1_MIN;
    if (minsSinceStart >= ctx->minsToControl - 1)
    {
        return ctx->tempProfile[ctx->minsToControl - 1];
    }

    float lowerTemp = ctx->tempProfile[minsSinceStart];
    float upperTemp = ctx->tempProfile[minsSinceStart + 1];
    float fraction = (float)(usSinceStart % USECONDS_IN_1_MIN) / USECONDS_IN_1_MIN;

    return (lowerTemp + ((upperTemp - lowerTemp) * fraction));
}