#ifndef ENDPOINT_DETECTOR_H
#define ENDPOINT_DETECTOR_H

#include <stdbool.h>

#define TORQUE_NORMAL 10.0f
#define TORQUE_SPIKE 85.0f
#define TORQUE_THRESHOLD 50.0f

void endpoint_mock_init(void);

float endpoint_simulate_torque(float current_angle);

bool endpoint_calibrate(float *out_min_angle, float *out_max_angle);

bool endpoint_is_movement_allowed(float target_angle);

#endif // ENDPOINT_DETECTOR_H