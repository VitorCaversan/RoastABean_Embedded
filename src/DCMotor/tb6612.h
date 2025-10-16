#ifndef _TB6612_H_
#define _TB6612_H_

#include "driver/ledc.h"
#include "driver/gpio.h"
#include <stdbool.h>

typedef enum EN_tbMotorId
{
    MOTOR_A = 0,
    MOTOR_B = 1
} EN_tbMotorId;

typedef enum EN_tbDir
{
    TB_DIR_FORWARD = 0,
    TB_DIR_REVERSE = 1
} EN_tbDir;

typedef struct tb6612Config_t{
    int stbyGpio; // Control pins

    int ain1Gpio, ain2Gpio, pwmaGpio; // Motor A pins

    int bin1Gpio, bin2Gpio, pwmbGpio; // Motor B pins

    // LEDC config (shared frequency)
    ledc_mode_t ledcMode;         // LEDC_LOW_SPEED_MODE is fine on S3
    ledc_timer_t ledcTimer;       // e.g. LEDC_TIMER_1
    uint32_t pwmFreqHz;           // e.g. 20000 (20 kHz)
    ledc_timer_bit_t dutyRes;     // e.g. LEDC_TIMER_10_BIT (0..1023)

    // LEDC channels (one per motor)
    ledc_channel_t chA;           // e.g. LEDC_CHANNEL_1
    ledc_channel_t chB;           // e.g. LEDC_CHANNEL_2
} tb6612Config_t;

typedef struct tb6612Handle_t{
    tb6612Config_t cfg;
    uint32_t maxDuty; // derived from dutyRes
} tb6612Handle_t;

// Initialize TB6612FNG control (LEDC + GPIOs). Leaves motors stopped & STBY enabled.
esp_err_t tb6612_init(tb6612Handle_t *h, const tb6612Config_t *cfg);

// Set direction (IN1/IN2). TB_DIR_FORWARD = IN1=1, IN2=0 ; TB_DIR_REVERSE = IN1=0, IN2=1
void tb6612_setDirection(tb6612Handle_t *h, EN_tbMotorId m, EN_tbDir dir);

// Coast (IN1=0, IN2=0) or Brake (IN1=1, IN2=1)
void tb6612_coast(tb6612Handle_t *h, EN_tbMotorId m);
void tb6612_brake(tb6612Handle_t *h, EN_tbMotorId m);

// Set duty 0..100 % (keeps current direction)
void tb6612_setDuty(tb6612Handle_t *h, EN_tbMotorId m, float dutyPercent);

// Convenience: set signed speed -100..100 (sign = direction)
void tb6612_setSpeed(tb6612Handle_t *h, EN_tbMotorId m, float signedPercent);

// Put driver in standby (low-power) or wake it
void tb6612_setStandby(tb6612Handle_t *h, bool standby);

#endif