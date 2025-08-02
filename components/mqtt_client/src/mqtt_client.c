/**
 * @file mqtt_client.c
 * @brief MQTT client for publishing device data as JSON.
 *
 * This module initializes and manages an MQTT publisher client using ESP-IDF
 * MQTT APIs. It supports JSON formatting with `cJSON`, and logs using the
 * `debugs` component.
 */

#include "mqtt_client.h"
#include "cJSON.h"
#include "debugs.h"
#include "my_mqtt_client.h"
#include <stdlib.h>
#include <string.h>

#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"

static esp_mqtt_client_handle_t client = NULL;

void mqtt_client_start(void) {
#if CONFIG_DEBUGS_ENABLE_LOG
  DEBUGS_LOGI("Initializing MQTT client...");
#endif

  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = MQTT_BROKER_URI,
  };

  client = esp_mqtt_client_init(&mqtt_cfg);
  if (client == NULL) {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGE("MQTT client initialization failed");
#endif
    return;
  }

  esp_err_t err = esp_mqtt_client_start(client);
  if (err != ESP_OK) {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGE("Failed to start MQTT client: %s", esp_err_to_name(err));
#endif
    return;
  }

#if CONFIG_DEBUGS_ENABLE_LOG
  DEBUGS_LOGI("MQTT client started");
#endif
}

void mqtt_client_publish_data(const device_data_t *data) {
  char *json_str = mqtt_client_build_json(data);
  if (json_str) {
    mqtt_client_publish_json("esp32/device/data", json_str);
    free(json_str);
  } else {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGE("Failed to build JSON string");
#endif
  }
}

void mqtt_client_publish_json(const char *topic, const char *json_str) {
  if (client == NULL) {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGW("MQTT client not initialized");
#endif
    return;
  }

  int msg_id = esp_mqtt_client_publish(client, topic, json_str, 0, 1, 0);
#if CONFIG_DEBUGS_ENABLE_LOG
  if (msg_id >= 0) {
    DEBUGS_LOGI("Published message (ID: %d) to topic: %s", msg_id, topic);
  } else {
    DEBUGS_LOGE("Failed to publish message to topic: %s", topic);
  }
#endif
}

char *mqtt_client_build_json(const device_data_t *data) {
  cJSON *root = cJSON_CreateObject();
  if (!root) {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGE("Failed to allocate cJSON root object");
#endif
    return NULL;
  }

  cJSON_AddStringToObject(root, "device_id", data->device_id);
  cJSON_AddBoolToObject(root, "fall_detected", data->fall_detected);
  cJSON_AddStringToObject(root, "timestamp", data->timestamp);
  cJSON_AddNumberToObject(root, "latitude", data->latitude);
  cJSON_AddNumberToObject(root, "longitude", data->longitude);

  char *json_str = cJSON_PrintUnformatted(root);
  if (!json_str) {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGE("Failed to print JSON string");
#endif
  }

  cJSON_Delete(root);
  return json_str;
}
