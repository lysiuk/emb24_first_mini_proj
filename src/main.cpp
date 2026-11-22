#include <stdint.h>
#include <inttypes.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Configuration

// LDR
#define LDR_ADC_UNIT        ADC_UNIT_1
#define LDR_ADC_CHANNEL     ADC_CHANNEL_8
#define LDR_ADC_ATTEN       ADC_ATTEN_DB_12
#define LDR_ADC_BITWIDTH    ADC_BITWIDTH_12

#define LIGHT_MIN_MV        50
#define LIGHT_MAX_MV        3000

// Servo
#define SERVO_GPIO          14
#define SERVO_FREQ_HZ       50
#define SERVO_TIMER         LEDC_TIMER_0
#define SERVO_CHANNEL       LEDC_CHANNEL_0
#define SERVO_RESOLUTION    LEDC_TIMER_12_BIT
#define SERVO_MODE          LEDC_LOW_SPEED_MODE

#define SERVO_MIN_DEG       10
#define SERVO_MAX_DEG       170

#define SERVO_MIN_US        500
#define SERVO_MAX_US        2500

#define SERVO_PERIOD_US     (1000000U / SERVO_FREQ_HZ)
#define SERVO_PWM_STEPS     (1U << SERVO_RESOLUTION)

// SMA
#define SMA_WINDOW_SIZE     10

// Superloop
#define LOOP_DELAY_MS       100

static const char *TAG = "LDR_SERVO";

// Data structures

typedef struct {
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t cali_handle;
} ldr_t;

typedef struct {
    int angle;
    uint32_t pulse_us;
    uint32_t duty;
} servo_command_t;

typedef struct {
    int buffer[SMA_WINDOW_SIZE];
    int32_t sum;
    uint8_t index;
    uint8_t count;
} sma_filter_t;

// Utility functions

static int clamp_int(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static int map_clamped(
    int value,
    int in_min,
    int in_max,
    int out_min,
    int out_max
)
{
    if (in_min >= in_max) {
        return out_min;
    }

    value = clamp_int(value, in_min, in_max);

    return out_min +
        (value - in_min) * (out_max - out_min) /
        (in_max - in_min);
}

// SMA Filter

static void sma_init(sma_filter_t *filter)
{
    *filter = {};
}

static int sma_update(sma_filter_t *filter, int value)
{
    if (filter->count == SMA_WINDOW_SIZE) {
        filter->sum -= filter->buffer[filter->index];
    } else {
        filter->count++;
    }

    filter->buffer[filter->index] = value;
    filter->sum += value;

    filter->index =
        (filter->index + 1) % SMA_WINDOW_SIZE;

    return filter->sum / filter->count;
}

// LDR

static void ldr_init(ldr_t *ldr)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = LDR_ADC_UNIT;

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(
            &unit_cfg,
            &ldr->adc_handle
        )
    );

    adc_oneshot_chan_cfg_t channel_cfg = {};
    channel_cfg.atten = LDR_ADC_ATTEN;
    channel_cfg.bitwidth = LDR_ADC_BITWIDTH;

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            ldr->adc_handle,
            LDR_ADC_CHANNEL,
            &channel_cfg
        )
    );

    adc_cali_curve_fitting_config_t cali_cfg = {};
    cali_cfg.unit_id = LDR_ADC_UNIT;
    cali_cfg.chan = LDR_ADC_CHANNEL;
    cali_cfg.atten = LDR_ADC_ATTEN;
    cali_cfg.bitwidth = LDR_ADC_BITWIDTH;

    ESP_ERROR_CHECK(
        adc_cali_create_scheme_curve_fitting(
            &cali_cfg,
            &ldr->cali_handle
        )
    );

    ESP_LOGI(TAG, "LDR initialized");
}

static esp_err_t ldr_read(
    ldr_t *ldr,
    int *raw,
    int *voltage_mv
)
{
    esp_err_t err = adc_oneshot_read(
        ldr->adc_handle,
        LDR_ADC_CHANNEL,
        raw
    );

    if (err != ESP_OK) {
        return err;
    }

    return adc_cali_raw_to_voltage(
        ldr->cali_handle,
        *raw,
        voltage_mv
    );
}

// Servo

static void servo_init(void)
{
    ledc_timer_config_t timer_cfg = {};

    timer_cfg.speed_mode = SERVO_MODE;
    timer_cfg.timer_num = SERVO_TIMER;
    timer_cfg.duty_resolution = SERVO_RESOLUTION;
    timer_cfg.freq_hz = SERVO_FREQ_HZ;
    timer_cfg.clk_cfg = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_cfg)
    );

    ledc_channel_config_t channel_cfg = {};

    channel_cfg.speed_mode = SERVO_MODE;
    channel_cfg.channel = SERVO_CHANNEL;
    channel_cfg.timer_sel = SERVO_TIMER;
    channel_cfg.intr_type = LEDC_INTR_DISABLE;
    channel_cfg.gpio_num = SERVO_GPIO;
    channel_cfg.duty = 0;
    channel_cfg.hpoint = 0;

    ESP_ERROR_CHECK(
        ledc_channel_config(&channel_cfg)
    );

    ESP_LOGI(TAG, "Servo initialized");
}

static servo_command_t servo_from_light(int voltage_mv)
{
    servo_command_t cmd = {};

    cmd.angle = map_clamped(
        voltage_mv,
        LIGHT_MIN_MV,
        LIGHT_MAX_MV,
        SERVO_MIN_DEG,
        SERVO_MAX_DEG
    );

    cmd.pulse_us = (uint32_t)map_clamped(
        cmd.angle,
        SERVO_MIN_DEG,
        SERVO_MAX_DEG,
        SERVO_MIN_US,
        SERVO_MAX_US
    );

    cmd.duty = (uint32_t)(
        ((uint64_t)cmd.pulse_us * SERVO_PWM_STEPS
            + SERVO_PERIOD_US / 2U)
        / SERVO_PERIOD_US
    );

    return cmd;
}

static esp_err_t servo_write(const servo_command_t *cmd)
{
    return ledc_set_duty_and_update(
        SERVO_MODE,
        SERVO_CHANNEL,
        cmd->duty,
        0
    );
}

extern "C" void app_main(void)
{
    ldr_t ldr = {};
    sma_filter_t light_filter = {};

    ldr_init(&ldr);
    servo_init();
    sma_init(&light_filter);

    ESP_LOGI(TAG, "System started");

    while (true) {
        int raw = 0;
        int voltage_mv = 0;

        esp_err_t err = ldr_read(
            &ldr,
            &raw,
            &voltage_mv
        );

        if (err == ESP_OK) {
            int filtered_mv = sma_update(
                &light_filter,
                voltage_mv
            );

            servo_command_t cmd =
                servo_from_light(filtered_mv);

            ESP_ERROR_CHECK(
                servo_write(&cmd)
            );

            ESP_LOGI(
                TAG,
                "ADC: %d | Voltage: %d mV | "
                "SMA: %d mV | Angle: %d | "
                "Pulse: %" PRIu32 " us | "
                "Duty: %" PRIu32,
                raw,
                voltage_mv,
                filtered_mv,
                cmd.angle,
                cmd.pulse_us,
                cmd.duty
            );
        } else {
            ESP_LOGW(
                TAG,
                "ADC read failed: %s",
                esp_err_to_name(err)
            );
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
    }
}