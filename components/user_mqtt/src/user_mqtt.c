#include "user_mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdlib.h> // Added for malloc, free
#include <string.h> // Added for string functions

#include "data_manager.h"
#include "json_wrapper.h"

static const char *TAG = "USER_MQTT";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    data_manager_set_mqtt_status(true);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    data_manager_set_mqtt_status(false);
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

  // NOTE: Consider using Kconfig for broker uri, last will topic/msg
  const esp_mqtt_client_config_t mqtt_cfg = {
      .broker = {.address.uri = broker_uri},
      .credentials =
          {
              .username = "",                     // Set via Kconfig
              .authentication = {.password = ""}, // Set via Kconfig
          },
      .session = {.last_will = {
                      .topic = "last_will_topic", // Set via Kconfig
                      .msg = "Disconnected",      // Set via Kconfig
                      .qos = 1,
                      .retain = 1,
                  }}};

  s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (s_mqtt_client == NULL) {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return ESP_FAIL;
  }

  esp_err_t err = esp_mqtt_client_register_event(
      s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register MQTT event handler");
    return err;
  }

  return esp_mqtt_client_start(s_mqtt_client);
}

/**
 * @brief Returns the global MQTT client handle.
 *
 * This function is used by other components to get the client handle.
 * @return esp_mqtt_client_handle_t The MQTT client handle.
 */
esp_mqtt_client_handle_t user_mqtt_get_client(void) { return s_mqtt_client; }
