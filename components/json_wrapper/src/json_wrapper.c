#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "data_manager.h"
#include "esp_log.h"
#include "json_wrapper.h"

static const char *TAG = "JSON_WRAPPER";

/**
 * @brief Creates a JSON payload string for a periodic status update.
 *
 * This function retrieves the current device state from the data_manager
 * and returns a dynamically allocated JSON string.
 *
 * @return A pointer to the dynamically allocated JSON string on success, or
 * NULL.
 */
char *json_wrapper_create_status_payload(void) {
  // 1. Get data from the Data Manager
  device_state_t data;
  esp_err_t err = data_manager_get_device_state(&data);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get device data from Data Manager: %d", err);
    return NULL;
  }

  // 2. Create the JSON object
  cJSON *root = cJSON_CreateObject();
  if (!root) {
    ESP_LOGE(TAG, "Failed to create JSON object");
    return NULL;
  }

  // Use cJSON_AddNumberToObject directly for 64-bit integer
  cJSON_AddNumberToObject(root, "timestamp", (double)data.timestamp_ms);

  // 3. Add all data fields for a status update
  cJSON_AddStringToObject(root, "device_id", data.device_id);
  cJSON_AddBoolToObject(root, "fall_detected", data.fall_detected);
  cJSON_AddNumberToObject(root, "latitude", data.gps_data.latitude);
  cJSON_AddNumberToObject(root, "longitude", data.gps_data.longitude);
  cJSON_AddBoolToObject(root, "has_gps_fix", data.gps_data.has_gps_fix);

  // 4. Print the JSON object to a string
  char *json_str = cJSON_PrintUnformatted(root);
  if (!json_str) {
    ESP_LOGE(TAG, "Failed to print JSON string");
  }

  // 5. Free the memory used by cJSON
  cJSON_Delete(root);

  ESP_LOGI(TAG, "Created status payload: %s", json_str);
  return json_str;
}

/**
 * @brief Creates a JSON payload string for a fall alert.
 *
 * This function retrieves the current device state and creates a specific
 * alert payload.
 *
 * @return A pointer to the dynamically allocated JSON string on success, or
 * NULL.
 */
char *json_wrapper_create_alert_payload(void) {
  // 1. Get data from the Data Manager
  device_state_t data;
  esp_err_t err = data_manager_get_device_state(&data);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get device data from Data Manager: %d", err);
    return NULL;
  }

  // 2. Create the JSON object
  cJSON *root = cJSON_CreateObject();
  if (!root) {
    ESP_LOGE(TAG, "Failed to create JSON object");
    return NULL;
  }

  // Add only the essential data for an alert
  cJSON_AddNumberToObject(root, "timestamp", (double)data.timestamp_ms);
  cJSON_AddStringToObject(root, "device_id", data.device_id);
  cJSON_AddBoolToObject(root, "fall_detected", data.fall_detected);

  // Add GPS data if a fix exists, otherwise add a message
  if (data.gps_data.has_gps_fix) {
    cJSON_AddNumberToObject(root, "latitude", data.gps_data.latitude);
    cJSON_AddNumberToObject(root, "longitude", data.gps_data.longitude);
  } else {
    cJSON_AddStringToObject(root, "message",
                            "Fall detected, GPS data unavailable.");
  }

  // 3. Print the JSON object to a string
  char *json_str = cJSON_PrintUnformatted(root);
  if (!json_str) {
    ESP_LOGE(TAG, "Failed to print JSON string");
  }

  // 4. Free the memory used by cJSON
  cJSON_Delete(root);

  ESP_LOGI(TAG, "Created alert payload: %s", json_str);
  return json_str;
}
