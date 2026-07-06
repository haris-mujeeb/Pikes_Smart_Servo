#include <unity.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "TEST_LOGGING";

// required by the framework
void setUp(void) {}
void tearDown(void) {}

void test_logging_output(void) {
    
    // Print a few different severity levels
    ESP_LOGI(TAG, "✅ INFO: If you can read this, your USB logging is working perfectly!");
    ESP_LOGW(TAG, "⚠️ WARNING: Just testing the yellow warning text.");
    ESP_LOGE(TAG, "❌ ERROR: Just testing the red error text.");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Log messages were fired.");
}

void app_main(void) {
    // Give it 1 seconds to connect 
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_logging_output);
    
    UNITY_END();
}