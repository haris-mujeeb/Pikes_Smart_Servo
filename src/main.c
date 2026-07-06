#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "USB_UART";

// Use UART0, which is intrinsically tied to the default USB console
#define UART_NUM        UART_NUM_0 
#define BUF_SIZE        1024

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing UART driver on default USB port (UART0)");

    // Install the UART driver. 
    // We do not need uart_param_config or uart_set_pin because UART0 
    // is already configured by the ESP-IDF bootloader.
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    uint8_t data[128]; // Buffer for incoming bytes

    ESP_LOGI(TAG, "Initialization complete. Waiting for data...");
    
    while (1) {
        // Publish data to the default USB-UART
        const char *msg = "Hello World from default USB-UART!\n";
        uart_write_bytes(UART_NUM, msg, strlen(msg));

        // Read data from the UART buffer
        // Wait for up to 20 ticks for data to arrive
        int length = uart_read_bytes(UART_NUM, data, (sizeof(data) - 1), 20 / portTICK_PERIOD_MS);

        if (length > 0) {
            data[length] = '\0'; // Null-terminated
            
            // Clean up invisible newline characters that the Serial Monitor might send
            data[strcspn((char*)data, "\r\n")] = 0; 
            
            ESP_LOGI(TAG, "Received %d bytes: %s", length, (char *)data);
        }

        // Delay for 1 second before publishing the next "Hello World"
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}