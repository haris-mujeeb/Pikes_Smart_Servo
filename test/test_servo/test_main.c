#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo_driver.h" 

#define TEST_SERVO_PIN 13

void setUp(void) {}
void tearDown(void) {}

void test_servo_initialization(void) {
    servo_init(TEST_SERVO_PIN, SERVO_MODE_180_DEGREE);
}

void test_servo_sweep(void) {
    float test_angles[] = {10.0, 90.0, 180.0, 90.0, 10.0};
    int num_angles = sizeof(test_angles) / sizeof(test_angles[0]);

    for (int i = 0; i < num_angles; i++) {
        // Test the control function from your library
        TEST_ASSERT_EQUAL(ESP_OK, servo_set_angle(test_angles[i]));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000)); 
    UNITY_BEGIN();
    RUN_TEST(test_servo_initialization);
    RUN_TEST(test_servo_sweep);
    UNITY_END(); 
}