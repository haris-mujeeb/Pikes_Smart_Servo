#include "endpoint_detector.h"
#include "esp_log.h"
#include <stdlib.h>
#include <time.h>

static const char *TAG = "ENDPOINT_DETECTOR";

static float true_min_limit = 0.0f;
static float true_max_limit = 60.0f;

static float calibrated_min = 0.0f;
static float calibrated_max = 180.0f;
static bool is_calibrated = false;

void endpoint_mock_init(void) {
    srand(time(NULL));
    
    // Generate a random starting point between 10 and 100 degrees
    true_min_limit = 10.0f + (float)(rand() % 90);
    
    // The max endpoints within a range of 60 degrees
    true_max_limit = true_min_limit + 60.0f; 
    
    ESP_LOGI(TAG, "Mock physical limits set randomly: Min = %.1f, Max = %.1f", true_min_limit, true_max_limit);
}

float endpoint_simulate_torque(float current_angle) {
    if (current_angle <= true_min_limit || current_angle >= true_max_limit) {
        return TORQUE_SPIKE; // Simulates the servo hitting the door frame
    }
    return TORQUE_NORMAL; // Simulates free movement
}

bool endpoint_calibrate(float *out_min_angle, float *out_max_angle) {
    ESP_LOGI(TAG, "Starting endpoint calibration routine...");
    
    float test_angle = 90.0f;
    float step = 1.0f;
    
    // find the minimum endpoint
    while (test_angle > 0.0f) {
        float torque = endpoint_simulate_torque(test_angle);
        if (torque > TORQUE_THRESHOLD) {
            calibrated_min = test_angle;
            ESP_LOGW(TAG, "Min endpoint detected at %.1f degrees (Torque: %.1f)", calibrated_min, torque);
            break;
        }
        test_angle -= step;
    }
    
    // reset to middle
    test_angle = 90.0f; 
    
    // find the maximum endpoint
    while (test_angle < 180.0f) {
        float torque = endpoint_simulate_torque(test_angle);
        if (torque > TORQUE_THRESHOLD) {
            calibrated_max = test_angle;
            ESP_LOGW(TAG, "Max endpoint detected at %.1f degrees (Torque: %.1f)", calibrated_max, torque);
            break;
        }
        test_angle += step;
    }
    
    *out_min_angle = calibrated_min;
    *out_max_angle = calibrated_max;
    is_calibrated = true;
    
    ESP_LOGI(TAG, "Calibration complete. Operational range: %.1f to %.1f", calibrated_min, calibrated_max);
    return true;
}

bool endpoint_is_movement_allowed(float target_angle) {
    if (!is_calibrated) return true; // allow movement if not yet restricted
    
    if (target_angle < calibrated_min) {
        ESP_LOGE(TAG, "Movement blocked: %.1f is below min limit (%.1f)", target_angle, calibrated_min);
        return false;
    }
    if (target_angle > calibrated_max) {
        ESP_LOGE(TAG, "Movement blocked: %.1f is above max limit (%.1f)", target_angle, calibrated_max);
        return false;
    }
    return true;
}