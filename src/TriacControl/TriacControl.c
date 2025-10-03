/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include "TriacControl.h"

/*******************************************************************************
 * MACROS AND DEFINES
 ******************************************************************************/

#define TRIAC_GPIO            GPIO_NUM_17
#define TRIAC_MODE            LEDC_LOW_SPEED_MODE   // low-speed works on all pins on S3
#define TRIAC_TIMER           LEDC_TIMER_1
#define TRIAC_CHANNEL         LEDC_CHANNEL_1
#define TRIAC_FREQ_HZ         25000                 // e.g., 25 kHz (above audible)
#define TRIAC_DUTY_RES        LEDC_TIMER_10_BIT     // 10-bit => duty range 0..1023

/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************/

static const char *TAG = "TRIAC";

/*******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/


/*******************************************************************************
 * EXTERNAL FUNCTIONS
 ******************************************************************************/

extern void triac_triacInit(void)
{
    ledc_timer_config_t tcfg = {
        .speed_mode       = TRIAC_MODE,
        .duty_resolution  = TRIAC_DUTY_RES,
        .timer_num        = TRIAC_TIMER,
        .freq_hz          = TRIAC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_channel_config_t ccfg = {
        .gpio_num         = TRIAC_GPIO,
        .speed_mode       = TRIAC_MODE,
        .channel          = TRIAC_CHANNEL,
        .intr_type        = LEDC_INTR_DISABLE,
        .timer_sel        = TRIAC_TIMER,
        .duty             = 0,           // start at 0%
        .hpoint           = 0
    };
    ledc_timer_config(&tcfg);
    ledc_channel_config(&ccfg);
}

extern void triac_setPwmPercent(float pwm)
{
    if (pwm < 0)
        pwm = 0;
    if (pwm > 100)
        pwm = 100;
    
    unsigned long maxDuty = (1u << TRIAC_DUTY_RES) - 1;    // e.g., 1023
    unsigned long duty = (unsigned long)((pwm / 100.0f) * (float)maxDuty + 0.5f);
    ledc_set_duty(TRIAC_MODE, TRIAC_CHANNEL, duty);
    ledc_update_duty(TRIAC_MODE, TRIAC_CHANNEL);
}

extern void triac_stopChannel(void)
{
    ledc_stop(TRIAC_MODE, TRIAC_CHANNEL, 0);
}