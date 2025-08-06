#include <string.h>
#include <stdlib.h>
#include "user_mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"

// Thêm các dependency đã được refactor
#include "data_manager.h"
#include "json_wrapper.h"

static const char *TAG = "USER_MQTT";

// Biến global để giữ handle của client MQTT
static esp_mqtt_client_handle_t s_mqtt_client = NULL;

/**
 * @brief Prototype mới cho event handler theo ESP-IDF v5.x.
 * @note  Đây là hàm xử lý các sự kiện từ MQTT client.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    //esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // Có thể publish một thông điệp chào mừng tại đây
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA. Topic: %.*s, Data: %.*s",
                     event->topic_len, event->topic, event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->esp_tls_stack_err != 0) {
                ESP_LOGE(TAG, "TLS stack error: %d", event->error_handle->esp_tls_stack_err);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other MQTT event id: %d", event->event_id);
            break;
    }
}

esp_err_t user_mqtt_init(const char *broker_uri) {
    if (broker_uri == NULL) {
        ESP_LOGE(TAG, "Broker URI is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Cấu trúc khởi tạo đúng chuẩn ESP-IDF v5.x
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = broker_uri,
        },
        .credentials = {
            // Tùy chọn: đặt username và password nếu cần
            .username = "",
            .authentication = {
                .password = "",
            },
        },
        .session = {
            .last_will = {
                .topic = "last_will_topic",
                .msg = "Disconnected",
                .qos = 1,
                .retain = 1,
            }
        }
    };

    // Khởi tạo client
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    // Đăng ký event handler theo cách mới của v5.x
    esp_err_t err = esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");
        return err;
    }

    // Bắt đầu client
    return esp_mqtt_client_start(s_mqtt_client);
}

esp_err_t user_mqtt_publish_current_data(const char *topic, int qos, int retain) {
    if (s_mqtt_client == NULL || topic == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized or topic is NULL");
        return ESP_ERR_INVALID_STATE;
    }

    // Lấy dữ liệu mới nhất và tạo payload JSON
    char *json_payload = json_wrapper_create_payload();
    if (json_payload == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON payload");
        return ESP_FAIL;
    }

    // Publish thông điệp
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, json_payload, 0, qos, retain);

    // Giải phóng bộ nhớ của payload sau khi publish
    free(json_payload);

    if (msg_id == -1) {
        ESP_LOGE(TAG, "Failed to publish message");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Published message ID: %d", msg_id);
    return ESP_OK;
}
