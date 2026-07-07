#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "endpoint_detector.h"
#include "esp_log.h"

void setUp(void) {
    // init random endpoints before each test
    endpoint_mock_init(); 
}

void tearDown(void) {}

void test_torque_simulation(void) {
    // We expect the calibration to find a 60 degree range
    float min_angle, max_angle;
    endpoint_calibrate(&min_angle, &max_angle);
    
    // Assert the range is exactly 60 degrees
    float range = max_angle - min_angle;
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 60.0f, range);
}

void test_movement_restriction(void) {
    float min_angle, max_angle;
    endpoint_calibrate(&min_angle, &max_angle);
    
    // Calculate a safe angle exactly in the middle of the range
    float safe_angle = min_angle + 30.0f;
    
    TEST_ASSERT_TRUE(endpoint_is_movement_allowed(safe_angle));
    TEST_ASSERT_FALSE(endpoint_is_movement_allowed(min_angle - 5.0f));
    TEST_ASSERT_FALSE(endpoint_is_movement_allowed(max_angle + 5.0f));
}

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000)); 
    
    UNITY_BEGIN();
    
    RUN_TEST(test_torque_simulation);
    RUN_TEST(test_movement_restriction);
    
    UNITY_END(); 
}