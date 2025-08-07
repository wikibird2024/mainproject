#include <stdio.h>  // Required for sprintf
#include <string.h> // Required for strlen, etc.

#include "cJSON.h"
#include "data_manager.h" // Use the new module
#include "esp_log.h"
#include "json_wrapper.h"

static const char *TAG = "JSON_WRAPPER";

/**
 * @brief Creates a JSON payload string from the latest device data.
 *
 * This function retrieves the current device state from the data_manager,
 * converts it into a cJSON object, and then prints it to an unformatted
 * JSON string. The returned string must be freed by the caller.
 *
 * @return A pointer to the dynamically allocated JSON string on success,
 * or NULL on failure.
 */
char *json_wrapper_create_payload(void) {
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

  // 3. Process and add the timestamp as a string to the JSON object
  char timestamp_str[20];
  sprintf(timestamp_str, "%lu", (unsigned long)data.timestamp_ms);
  cJSON_AddStringToObject(root, "timestamp", timestamp_str);

  // 4. Add other data fields to the JSON object
  cJSON_AddStringToObject(root, "device_id", data.device_id);
  cJSON_AddBoolToObject(root, "fall_detected", data.fall_detected);
  cJSON_AddNumberToObject(root, "latitude", data.gps_data.latitude);
  cJSON_AddNumberToObject(root, "longitude", data.gps_data.longitude);

  // 5. Print the JSON object to a string
  char *json_str = cJSON_PrintUnformatted(root);
  if (!json_str) {
    ESP_LOGE(TAG, "Failed to print JSON string");
    cJSON_Delete(root);
    return NULL;
  }

  // 6. Free the memory used by cJSON
  cJSON_Delete(root);

  ESP_LOGI(TAG, "Created JSON payload: %s", json_str);
  return json_str; // This string must be freed by the caller
}
