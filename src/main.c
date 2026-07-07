#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "servo_driver.h"
#include "endpoint_detector.h"

static const char *TAG = "MAIN_APP";
#define SERVO_PIN 13
#define SERVO_MODE SERVO_MODE_180_DEGREE

bool ready_message_printed = false;

typedef enum {
    STATE_INIT,
    STATE_CALIBRATE_MIN,
    STATE_CALIBRATE_MAX,
    STATE_OPERATIONAL,
    STATE_ERROR
} system_state_t;

void __attribute__((weak)) app_main(void) {
    ESP_LOGI(TAG, "Starting PIKES Smart Servo Application...");

    system_state_t current_state = STATE_INIT;
    float current_angle = 90.0f;
    float detected_min = 10.0f;
    float detected_max = 180.0f;
    float target_door_velocity = 0.0f;

    while (1) {       
        switch (current_state) {
            case STATE_INIT:
                ESP_LOGI(TAG, "STATE: INIT");
                servo_init(SERVO_PIN, SERVO_MODE);
                endpoint_mock_init(); 
                
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    current_angle = 90.0f;
                    servo_set_angle(current_angle);
                } else {
                    current_angle = 90.0f; // Assume we start in the middle
                    servo_set_speed(0.0f); // Stop the 360 servo
                }
                
                vTaskDelay(pdMS_TO_TICKS(1500)); 
                current_state = STATE_CALIBRATE_MIN;
                ESP_LOGI(TAG, "Transitioning to: CALIBRATE_MIN");
                break;

            case STATE_CALIBRATE_MIN:
                // Move backwards
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    current_angle -= 1.0f;
                    servo_set_angle(current_angle);
                } else {
                    target_door_velocity = -25.0f; // 25% speed reverse
                    float motor_speed = servo_apply_transmission_ratio(target_door_velocity); // Apply 40:15 ratio
                    servo_set_speed(motor_speed);
                    current_angle -= 1.0f; // Simulate angle tracking
                }
                
                // Stop condition to terminate the endpoint detection process[cite: 1]
                if (endpoint_simulate_torque(current_angle) > TORQUE_THRESHOLD) {
                    detected_min = current_angle;
                    ESP_LOGE(TAG, "COLLISION! Min endpoint found at %.1f deg", detected_min);
                    
                    if (SERVO_MODE == SERVO_MODE_360_CONTINUOUS) servo_set_speed(0.0f);
                    
                    // Reset to center
                    current_angle = 90.0f;
                    if (SERVO_MODE == SERVO_MODE_180_DEGREE) servo_set_angle(current_angle);
                    
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    current_state = STATE_CALIBRATE_MAX;
                    ESP_LOGI(TAG, "Transitioning to: CALIBRATE_MAX");
                }
                break;

            case STATE_CALIBRATE_MAX:
                // Move forwards
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    current_angle += 1.0f;
                    servo_set_angle(current_angle);
                } else {
                    target_door_velocity = 25.0f; // 25% speed forward
                    float motor_speed = servo_apply_transmission_ratio(target_door_velocity); // Apply 40:15 ratio[cite: 1]
                    servo_set_speed(motor_speed);
                    current_angle += 1.0f; // Simulate angle tracking
                }
                
                // Stop condition to terminate the endpoint detection process[cite: 1]
                if (endpoint_simulate_torque(current_angle) > TORQUE_THRESHOLD) {
                    detected_max = current_angle;
                    ESP_LOGE(TAG, "COLLISION! Max endpoint found at %.1f deg", detected_max);
                    
                    if (SERVO_MODE == SERVO_MODE_360_CONTINUOUS) servo_set_speed(0.0f);
                    
                    current_angle = detected_min;
                    target_door_velocity = 25.0f; // Start operational sweep going forward
                    
                    current_state = STATE_OPERATIONAL;
                    ESP_LOGI(TAG, "Calibration Complete! Safe Range: %.1f -> %.1f", detected_min, detected_max);
                    ESP_LOGI(TAG, "Transitioning to: OPERATIONAL");
                }
                break;

            case STATE_OPERATIONAL:
                if (!ready_message_printed) {
                        ESP_LOGI(TAG, "System ready and holding position. Awaiting ROS 2 / MQTT commands...");
                        ready_message_printed = true;
                }
                break;

            case STATE_ERROR:
                ESP_LOGE(TAG, "CRITICAL ERROR: Limits exceeded.");
                if (SERVO_MODE == SERVO_MODE_360_CONTINUOUS) servo_set_speed(0.0f);
                while(1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(30)); 
        }
}