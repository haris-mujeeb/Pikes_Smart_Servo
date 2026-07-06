#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include "esp_err.h"
#include <stdint.h>

typedef enum {
    SERVO_MODE_180_DEGREE,
    SERVO_MODE_360_CONTINUOUS
} servo_mode_t;

esp_err_t servo_init(int gpio_pin, servo_mode_t mode);

esp_err_t servo_set_angle(float angle);

esp_err_t servo_set_speed(float speed_percent);

float servo_apply_transmission_ratio(float door_target);

#endif // SERVO_DRIVER_H