#include <unity.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_manager.h"

#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"

// Reference external function from main.c to configure network stack natively
extern void wifi_init_sta(void);

void setUp(void) {}
void tearDown(void) {}

void test_mqtt_teleop_handshake(void) {
    // 1. Initialize dependencies inside the test context
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
        ret = nvs_flash_init();
    }
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    wifi_init_sta();
    
    // Give the hardware 5 seconds to acquire DHCP IP assignment
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 2. Start the MQTT Manager
    mqtt_start(MQTT_BROKER_URI); // Match your broker IP address
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 3. Prompt interactive developer input via console output
    printf("\n=================================================================\n");
    printf("!!! Please make sure 'test_mqtt.py' is running on your host PC. It is located under './test/test_mqtt/'.\n");
    printf("=================================================================\n\n");

    bool handshake_success = false;
    int timeout_seconds = 20;

    // 4. Actively poll for incoming data packets over the broker bridge
    for (int i = 0; i < timeout_seconds; i++) {
        float incoming_val = mqtt_get_servo_rot_vel();
        
        if (incoming_val != 0.0f) {
            printf("[SUCCESS] MQTT loopback validated! Received velocity: %.2f\n", incoming_val);
            handshake_success = true;
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Assert that the firmware successfully captured a command sent by the python loop
    TEST_ASSERT_TRUE_MESSAGE(handshake_success, "Test timed out. No input detected from the python script 'test_mqtt.py'.");
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000)); 
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_teleop_handshake);
    UNITY_END(); 
}