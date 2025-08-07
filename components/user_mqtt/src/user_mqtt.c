#include "user_mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>

// Add refactored dependencies
#include "data_manager.h"
#include "json_wrapper.h"

static const char *TAG = "USER_MQTT";

// Global variable to hold the MQTT client handle
static esp_mqtt_client_handle_t s_mqtt_client = NULL;

/**
 * @brief New prototype for the event handler according to ESP-IDF v5.x.
 * @note This function processes events from the MQTT client.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  // esp_mqtt_client_handle_t client = event->client;

  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    // A welcome message can be published here
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA. Topic: %.*s, Data: %.*s", event->topic_len,
             event->topic, event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    if (event->error_handle->esp_tls_stack_err != 0) {
      ESP_LOGE(TAG, "TLS stack error: %d",
               event->error_handle->esp_tls_stack_err);
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

  // Standard initialization structure for ESP-IDF v5.x
  const esp_mqtt_client_config_t mqtt_cfg = {
      .broker =
          {
              .address.uri = broker_uri,
          },
      .credentials =
          {
              // Optional: set username and password if needed
              .username = "",
              .authentication =
                  {
                      .password = "",
                  },
          },
      .session = {.last_will = {
                      .topic = "last_will_topic",
                      .msg = "Disconnected",
                      .qos = 1,
                      .retain = 1,
                  }}};

  // Initialize the client
  s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (s_mqtt_client == NULL) {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return ESP_FAIL;
  }

  // Register the event handler in the new v5.x way
  esp_err_t err = esp_mqtt_client_register_event(
      s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register MQTT event handler");
    return err;
  }

  // Start the client
  return esp_mqtt_client_start(s_mqtt_client);
}

esp_err_t user_mqtt_publish_current_data(const char *topic, int qos,
                                         int retain) {
  if (s_mqtt_client == NULL || topic == NULL) {
    ESP_LOGE(TAG, "MQTT client not initialized or topic is NULL");
    return ESP_ERR_INVALID_STATE;
  }

  // Get the latest data and create the JSON payload
  char *json_payload = json_wrapper_create_payload();
  if (json_payload == NULL) {
    ESP_LOGE(TAG, "Failed to create JSON payload");
    return ESP_FAIL;
  }

  // Publish the message
  int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, json_payload, 0,
                                       qos, retain);

  // Free the payload memory after publishing
  free(json_payload);

  if (msg_id == -1) {
    ESP_LOGE(TAG, "Failed to publish message");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Published message ID: %d", msg_id);
  return ESP_OK;
}
