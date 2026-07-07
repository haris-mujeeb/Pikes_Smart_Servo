#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h> // for usleep()

#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h" // requried by ESP-IDF WiFi driver
#include "servo_driver.h"
#include "endpoint_detector.h"
#include "mqtt_manager.h"

static const char *TAG = "MAIN_APP";
#define SERVO_PIN 13
#define SERVO_MODE SERVO_MODE_180_DEGREE

#define ENABLE_CALIBRATION
#define ENABLE_CLAMPING

#define WIFI_SSID      "HarisPCHotspot"             // !! Replace it
#define WIFI_PASS      "very_secure_password"       // !! Replace it
#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"  // !! Replace it

volatile bool wifi_connected = false;
bool ready_message_printed = false;

typedef enum {
    STATE_INIT,
#ifdef ENABLE_CALIBRATION
    STATE_CALIBRATE_MIN,
    STATE_CALIBRATE_MAX,
#endif
    STATE_OPERATIONAL,
    STATE_ERROR
} system_state_t;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected from Wi-Fi. Retrying...");
        esp_wifi_connect();
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Successfully connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

void wifi_init_sta(void) {
    wifi_connected = false;
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi Station initialization completed. Connecting...");
}

void __attribute__((weak)) app_main(void) {
    ESP_LOGI(TAG, "Starting PIKES Smart Servo Application...");

    // Initialize NVS (required for Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Note: Make sure NVS is initialized before running this !
    wifi_init_sta();

    ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
    while (!wifi_connected) {
        usleep(100 * 1000); // 100ms
    }

    mqtt_start(MQTT_BROKER_URI);

    system_state_t current_state = STATE_INIT;
    float current_angle = 90.0f;
    float detected_min = 10.0f;
    float detected_max = 180.0f;
    float target_door_velocity = 0.0f;

    const float SAFETY_MARGIN = 2.0f;

    while (1) {       
        switch (current_state) {
            case STATE_INIT:
                ESP_LOGI(TAG, "STATE: INIT");
                servo_init(SERVO_PIN, SERVO_MODE);
                endpoint_mock_init(); 
                
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    current_angle = 90.0f;
                    servo_set_angle(servo_apply_transmission_ratio(current_angle));
                } else {
                    current_angle = 90.0f; 
                    servo_set_speed(0.0f); 
                }
                
                usleep(1500 * 1000); // 1.5 second delay
#ifdef ENABLE_CALIBRATION
                current_state = STATE_CALIBRATE_MIN;
                ESP_LOGI(TAG, "Transitioning to: CALIBRATE_MIN");
#else
                current_state = STATE_OPERATIONAL;
                ESP_LOGI(TAG, "Calibration disabled. Using default bounds: %.1f -> %.1f", detected_min, detected_max);
                ESP_LOGI(TAG, "Transitioning to: OPERATIONAL");
#endif
                break;
#ifdef ENABLE_CALIBRATION
            case STATE_CALIBRATE_MIN:
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    current_angle -= 1.0f;
                    servo_set_angle(servo_apply_transmission_ratio(current_angle));
                } else {
                    target_door_velocity = -25.0f; 
                    float motor_speed = servo_apply_transmission_ratio(target_door_velocity); 
                    servo_set_speed(motor_speed);
                    current_angle -= 1.0f; 
                }
                
                if (endpoint_simulate_torque(current_angle) > TORQUE_THRESHOLD) {
                    detected_min = current_angle;
                    ESP_LOGE(TAG, "COLLISION! Min endpoint found at %.1f deg", detected_min);
                    
                    if (SERVO_MODE == SERVO_MODE_360_CONTINUOUS) servo_set_speed(0.0f);
                    
                    current_angle = 90.0f;
                    if (SERVO_MODE == SERVO_MODE_180_DEGREE) servo_set_angle(servo_apply_transmission_ratio(current_angle));
                    
                    usleep(1500 * 1000); // 1.5 second delay
                    current_state = STATE_CALIBRATE_MAX;
                    ESP_LOGI(TAG, "Transitioning to: CALIBRATE_MAX");
                }
                break;

            case STATE_CALIBRATE_MAX:
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    current_angle += 1.0f;
                    servo_set_angle(servo_apply_transmission_ratio(current_angle));
                } else {
                    target_door_velocity = 25.0f; 
                    float motor_speed = servo_apply_transmission_ratio(target_door_velocity); 
                    servo_set_speed(motor_speed);
                    current_angle += 1.0f; 
                }
                
                if (endpoint_simulate_torque(current_angle) > TORQUE_THRESHOLD) {
                    detected_max = current_angle;
                    ESP_LOGE(TAG, "COLLISION! Max endpoint found at %.1f deg", detected_max);
                    
                    if (SERVO_MODE == SERVO_MODE_360_CONTINUOUS) servo_set_speed(0.0f);
                    
                    current_angle = detected_min;
                    target_door_velocity = 25.0f; 
                    
                    current_state = STATE_OPERATIONAL;
                    ESP_LOGI(TAG, "Calibration Complete! Safe Range: %.1f -> %.1f", detected_min, detected_max);
                    ESP_LOGI(TAG, "Transitioning to: OPERATIONAL");
                }
                break;
#endif // ENABLE_CALIBRATION
            case STATE_OPERATIONAL:
                if (!ready_message_printed) {
                    current_angle = (detected_max + detected_min)/2;
                    ESP_LOGI(TAG, "System ready. Awaiting ROS 2 keyboard commands...");
                    ready_message_printed = true;
                }
                
                target_door_velocity = mqtt_get_servo_rot_vel();
                float angle_step = target_door_velocity / 25.0f;
                float next_angle = current_angle + angle_step;

                float safe_max = detected_max - SAFETY_MARGIN;
                float safe_min = detected_min + SAFETY_MARGIN;
#ifdef ENABLE_CLAMPING
                if (current_angle >= safe_max && target_door_velocity > 0) {
                    target_door_velocity = 0.0f;
                    current_angle = safe_max;
                } else if (current_angle <= safe_min && target_door_velocity < 0) {
                    target_door_velocity = 0.0f; 
                    current_angle = safe_min;
                } else {
                    current_angle = next_angle;
                }
#else
                current_angle = next_angle;
#endif // ENABLE_CLAMPING
                if (SERVO_MODE == SERVO_MODE_180_DEGREE) {
                    servo_set_angle(servo_apply_transmission_ratio(current_angle));
                } else {
                    float motor_speed = servo_apply_transmission_ratio(target_door_velocity); 
                    servo_set_speed(motor_speed);
                }

                static uint32_t publish_counter = 0;
                if (publish_counter++ % 10 == 0) { 
                    float current_torque = endpoint_simulate_torque(current_angle);
                    mqtt_pub_telemetry(current_angle, current_torque);
                }
                break;

            case STATE_ERROR:
                ESP_LOGE(TAG, "CRITICAL ERROR: Limits exceeded.");
                if (SERVO_MODE == SERVO_MODE_360_CONTINUOUS) servo_set_speed(0.0f);
                while(1) { usleep(1000 * 1000); } // 1 second block
                break;
        }

        // to transmit data every 10 cycle (~300ms)
        static uint32_t publish_counter = 0;
        if (publish_counter++ % 10 == 0) { 
            float current_torque = endpoint_simulate_torque(current_angle);
            mqtt_pub_telemetry(current_angle, current_torque);
        }

        usleep(30 * 1000); // 30ms
    }
}