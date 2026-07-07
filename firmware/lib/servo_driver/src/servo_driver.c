#include "servo_driver.h"
#include "driver/ledc.h" // for PWM
#include "esp_log.h"

static const char *TAG = "SERVO_DRIVER";

// Setting up the internal timer
#define SERVO_LEDC_TIMER    LEDC_TIMER_0
#define SERVO_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_CHANNEL  LEDC_CHANNEL_0
#define SERVO_DUTY_RES      LEDC_TIMER_13_BIT 
#define SERVO_FREQUENCY     50              
#define MAX_DUTY            8191              

static servo_mode_t current_servo_mode = SERVO_MODE_180_DEGREE;

static uint32_t calculate_duty_from_angle(float angle)
{
    if (angle < 0.0f)
        angle = 0.0f;
    else if (angle > 180.0f)
        angle = 180.0f;

    float pulse_width_ms = 1.0f + (angle / 180.0f);

    return (uint32_t)((pulse_width_ms * MAX_DUTY) / 20.0f);
}

static uint32_t calculate_duty_from_speed(float speed_percent) {
    // Clamp speed between -100% and 100%
    if (speed_percent < -100.0f) speed_percent = -100.0f;
    if (speed_percent > 100.0f) speed_percent = 100.0f;

    // Note: For a 360 servo: 1.5ms is stop. 1.0ms is full reverse. 2.0ms is full forward.
    // Map -100.0 to 100.0 percent directly to 1.0ms to 2.0ms
    float pulse_width_ms = 1.5f + (speed_percent / 100.0f) * 0.5f;
    return (uint32_t)((pulse_width_ms / 20.0f) * MAX_DUTY);
}

esp_err_t servo_init(int gpio_pin, servo_mode_t mode) {
    current_servo_mode = mode;
    ESP_LOGI(TAG, "Initializing servo driver on GPIO %d (Mode: %s)", 
             gpio_pin, mode == SERVO_MODE_360_CONTINUOUS ? "360 Continuous" : "180 Degree");

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = SERVO_LEDC_MODE,
        .timer_num        = SERVO_LEDC_TIMER,
        .duty_resolution  = SERVO_DUTY_RES,
        .freq_hz          = SERVO_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) return err;

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = SERVO_LEDC_MODE,
        .channel        = SERVO_LEDC_CHANNEL,
        .timer_sel      = SERVO_LEDC_TIMER,
        .gpio_num       = gpio_pin,
        .duty           = 0,
        .hpoint         = 0
    };
    
    return ledc_channel_config(&ledc_channel);
}

esp_err_t servo_set_angle(float angle) {
    if (current_servo_mode != SERVO_MODE_180_DEGREE) {
        ESP_LOGE(TAG, "Cannot set angle on a continuous rotation servo!");
        return ESP_ERR_INVALID_STATE;
    }

    static float last_applied_angle = -999.0f;
    if (last_applied_angle != -999.0f) {
        float diff = angle - last_applied_angle;
        if (diff < 0) diff = -diff;
        if (diff < 2.0f) {
            return ESP_OK;
        }
    }

    uint32_t duty = calculate_duty_from_angle(angle);
    ESP_LOGI(TAG, "Commanding Angle: %.1f deg -> PWM Duty Cycle: %lu", angle, (unsigned long)duty);
    
    esp_err_t err = ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    if (err != ESP_OK) return err;
    
    err = ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
    if (err == ESP_OK) {
        last_applied_angle = angle;
    }
    return err;
}

esp_err_t servo_set_speed(float speed_percent) {
    if (current_servo_mode != SERVO_MODE_360_CONTINUOUS) {
        ESP_LOGE(TAG, "Cannot set speed on a 180-degree positional servo!");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t duty = calculate_duty_from_speed(speed_percent);
    ESP_LOGI(TAG, "Commanding Speed: %.1f%% -> PWM Duty Cycle: %lu", speed_percent, (unsigned long)duty);
    
    esp_err_t err = ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    if (err != ESP_OK) return err;
    
    return ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
}

float servo_apply_transmission_ratio(float door_target) {
    // The transmission box is 40:15[cite: 1]
    // Motor Target = Door Target * (40 / 15)
    float gear_ratio = 40.0f / 15.0f;
    float motor_target = door_target * gear_ratio;
    
    ESP_LOGI(TAG, "Transmission applied: Door Target %.2f -> Motor Target %.2f", door_target, motor_target);
    return motor_target;
}