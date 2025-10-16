#include "tb6612.h"
#include <math.h>

static void setInPins(int in1, int in2, int v1, int v2) {
    gpio_set_level(in1, v1);
    gpio_set_level(in2, v2);
}

esp_err_t tb6612_init(tb6612Handle_t *h, const tb6612Config_t *cfg)
{
    if (!h || !cfg) return ESP_ERR_INVALID_ARG;
    h->cfg = *cfg;
    h->maxDuty = (1u << cfg->dutyRes) - 1u;

    // STBY + IN pins
    int pins[] = { cfg->stbyGpio, cfg->ain1Gpio, cfg->ain2Gpio, cfg->bin1Gpio, cfg->bin2Gpio };
    for (int i = 0; i < (int)(sizeof(pins)/sizeof(pins[0])); ++i) {
        if (pins[i] >= 0) {
            gpio_config_t io = {
                .pin_bit_mask = 1ULL << pins[i],
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            ESP_ERROR_CHECK(gpio_config(&io));
        }
    }

    // Enable driver (STBY=1)
    if (cfg->stbyGpio >= 0) gpio_set_level(cfg->stbyGpio, 1);

    // LEDC timer (shared)
    ledc_timer_config_t tcfg = {
        .speed_mode = cfg->ledcMode,
        .timer_num = cfg->ledcTimer,
        .duty_resolution = cfg->dutyRes,
        .freq_hz = cfg->pwmFreqHz,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    // LEDC channel A
    ledc_channel_config_t ccA = {
        .gpio_num = cfg->pwmaGpio,
        .speed_mode = cfg->ledcMode,
        .channel = cfg->chA,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = cfg->ledcTimer,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccA));

    // LEDC channel B
    ledc_channel_config_t ccB = {
        .gpio_num = cfg->pwmbGpio,
        .speed_mode = cfg->ledcMode,
        .channel = cfg->chB,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = cfg->ledcTimer,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccB));

    // Default to coast and 0% duty
    tb6612_coast(h, MOTOR_A);
    tb6612_coast(h, MOTOR_B);
    tb6612_setDuty(h, MOTOR_A, 0);
    tb6612_setDuty(h, MOTOR_B, 0);

    return ESP_OK;
}

void tb6612_setDirection(tb6612Handle_t *h, EN_tbMotorId m, EN_tbDir dir)
{
    if (!h) return;
    if (m == MOTOR_A) {
        setInPins(h->cfg.ain1Gpio, h->cfg.ain2Gpio, dir==TB_DIR_FORWARD, dir==TB_DIR_REVERSE);
    } else {
        setInPins(h->cfg.bin1Gpio, h->cfg.bin2Gpio, dir==TB_DIR_FORWARD, dir==TB_DIR_REVERSE);
    }
}

void tb6612_coast(tb6612Handle_t *h, EN_tbMotorId m)
{
    if (!h) return;
    if (m == MOTOR_A) setInPins(h->cfg.ain1Gpio, h->cfg.ain2Gpio, 0, 0);
    else             setInPins(h->cfg.bin1Gpio, h->cfg.bin2Gpio, 0, 0);
}

void tb6612_brake(tb6612Handle_t *h, EN_tbMotorId m)
{
    if (!h) return;
    if (m == MOTOR_A) setInPins(h->cfg.ain1Gpio, h->cfg.ain2Gpio, 1, 1);
    else             setInPins(h->cfg.bin1Gpio, h->cfg.bin2Gpio, 1, 1);
}

void tb6612_setDuty(tb6612Handle_t *h, EN_tbMotorId m, float dutyPercent)
{
    if (!h) return;
    if (dutyPercent < 0) dutyPercent = 0;
    if (dutyPercent > 100) dutyPercent = 100;
    uint32_t duty = (uint32_t)lroundf((dutyPercent / 100.0f) * (float)h->maxDuty);

    ledc_channel_t ch = (m == MOTOR_A) ? h->cfg.chA : h->cfg.chB;
    ledc_set_duty(h->cfg.ledcMode, ch, duty);
    ledc_update_duty(h->cfg.ledcMode, ch);
}

void tb6612_setSpeed(tb6612Handle_t *h, EN_tbMotorId m, float signedPercent)
{
    if (!h) return;
    EN_tbDir dir = (signedPercent >= 0) ? TB_DIR_FORWARD : TB_DIR_REVERSE;
    tb6612_setDirection(h, m, dir);
    tb6612_setDuty(h, m, fabsf(signedPercent));
}

void tb6612_setStandby(tb6612Handle_t *h, bool standby)
{
    if (!h) return;
    if (h->cfg.stbyGpio >= 0) gpio_set_level(h->cfg.stbyGpio, standby ? 0 : 1);
}
