#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "MQTT_MANAGER";

static esp_mqtt_client_handle_t client = NULL;
static volatile float current_target_velocity = 0.0f; // Volatile because it's updated in a background callback

// --- MQTT Event Handler ---
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected! Subscribing to /pikes/servo/cmd_vel");
            esp_mqtt_client_subscribe(event->client, "/pikes/servo/cmd_vel", 0);
            break;
        
        case MQTT_EVENT_DATA:
            // check if the message is on the topic
            if (strncmp(event->topic, "/pikes/servo/cmd_vel", event->topic_len) == 0) {
                
                // parse the JSON payload {"velocity": X}
                cJSON *json = cJSON_ParseWithLength(event->data, event->data_len);
                if (json != NULL) {
                    cJSON *vel_item = cJSON_GetObjectItem(json, "velocity");
                    if (cJSON_IsNumber(vel_item)) {
                        current_target_velocity = vel_item->valuedouble;
                        ESP_LOGI(TAG, "New Target Velocity: %.1f", current_target_velocity);
                    }
                    cJSON_Delete(json);
                }
            }
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected from broker.");
            break;
            
        default:
            break;
    }
}

// --- Public ---
void mqtt_start(const char *broker_uri) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
    };
    
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT client started targeting %s", broker_uri);
}

float mqtt_get_servo_rot_vel(void) {
    return current_target_velocity;
}

void mqtt_pub_telemetry(float angle, float torque) {
    if (client == NULL) return; // if not connected
    
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"angle\":%.1f, \"torque\":%.1f}", angle, torque);
    
    esp_mqtt_client_publish(client, "/pikes/servo/telemetry", payload, 0, 0, 0);
}