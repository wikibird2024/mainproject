#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_manager.h"
#include "data_manager_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "json_wrapper.h" // Provides json_wrapper_* functions
#include "mqtt_client.h"  // Provides esp_mqtt_client_publish
#include "sdkconfig.h"
#include "sim4g_at.h" // Provides sim4g_at_get_gps
#include "sim4g_gps.h"
#include "user_mqtt.h" // Provides user_mqtt_get_client

static const char *TAG = "SIM4G_GPS";

// Global variables to store SIM4G and GPS data
static char s_phone_number[16] = ""; // Phone number for SMS

// A mutex to protect access to the SIM4G AT module for GPS queries
static SemaphoreHandle_t s_gps_at_mutex;

#define MQTT_TASK_STACK_SIZE CONFIG_MQTT_TASK_STACK_SIZE
#define MQTT_TASK_PRIORITY CONFIG_MQTT_TASK_PRIORITY
#define ALERT_TASK_STACK_SIZE CONFIG_ALERT_TASK_STACK_SIZE
#define ALERT_TASK_PRIORITY CONFIG_ALERT_TASK_PRIORITY

// -----------------------------------------------------------------------------
// New Task for Fall Alert
// -----------------------------------------------------------------------------

/**
 * @brief FreeRTOS task to handle both SMS and MQTT alerts for a fall event.
 *
 * @param param A pointer to the gps_data_t struct from the moment of the fall.
 */
static void fall_alert_task(void *param) {
  gps_data_t *loc = (gps_data_t *)param;
  char msg[256];

  if (loc->has_gps_fix) {
    snprintf(msg, sizeof(msg), "Fall detected!\nLat: %.6f\nLon: %.6f\nTime: %s",
             loc->latitude, loc->longitude, loc->timestamp);
  } else {
    snprintf(msg, sizeof(msg), "Fall detected! GPS data unavailable.");
  }

  if (strlen(s_phone_number) > 0) {
    ESP_LOGI(TAG, "Sending SMS to %s:\n%s", s_phone_number, msg);
    esp_err_t sms_err = sim4g_at_send_sms(s_phone_number, msg);
    if (sms_err != ESP_OK) {
      ESP_LOGE(TAG, "SMS send failed: %s", esp_err_to_name(sms_err));
    } else {
      ESP_LOGI(TAG, "SMS sent successfully");
    }
  } else {
    ESP_LOGW(TAG, "SMS not sent, phone number is not set.");
  }

  // Update the data_manager with fall event and GPS data
  device_state_t current_state;
  data_manager_get_device_state(&current_state);
  current_state.gps_data = *loc;
  current_state.fall_detected = true;
  data_manager_set_device_state(&current_state);

  ESP_LOGI(TAG, "Publishing fall alert to MQTT...");
  char *json_payload = json_wrapper_create_alert_payload();
  if (json_payload) {
    int msg_id = esp_mqtt_client_publish(
        user_mqtt_get_client(), CONFIG_MQTT_ALERT_TOPIC, json_payload, 0, 1, 0);
    free(json_payload);
    if (msg_id == -1) {
      ESP_LOGE(TAG, "MQTT publish failed.");
    } else {
      ESP_LOGI(TAG, "MQTT alert published successfully, msg_id=%d", msg_id);
    }
  } else {
    ESP_LOGE(TAG, "Failed to create JSON alert payload.");
  }

  free(loc);
  vTaskDelete(NULL);
}
// -----------------------------------------------------------------------------
// Existing Tasks and Functions (with some modifications)
// -----------------------------------------------------------------------------

/**
 * @brief Public function to update GPS location from the SIM4G module.
 */
void sim4g_gps_update_location(void) {
  gps_data_t new_gps_data;
  if (xSemaphoreTake(s_gps_at_mutex, portMAX_DELAY) == pdTRUE) {
    sim4g_at_get_gps(&new_gps_data);
    data_manager_set_gps_data(&new_gps_data);
    xSemaphoreGive(s_gps_at_mutex);
  }
}

/**
 * @brief FreeRTOS task for periodically reading GPS data and publishing to
 * MQTT.
 */
static void mqtt_monitoring_task(void *param) {
  ESP_LOGI(TAG, "MQTT monitoring task started");
  while (1) {
    sim4g_gps_update_location();

    if (data_manager_get_mqtt_status()) {
      ESP_LOGI(TAG, "Publishing periodic data to MQTT...");
      char *json_payload = json_wrapper_create_status_payload();
      if (json_payload) {
        int msg_id = esp_mqtt_client_publish(user_mqtt_get_client(),
                                             CONFIG_MQTT_STATUS_TOPIC,
                                             json_payload, 0, 0, 0);
        free(json_payload);
        if (msg_id == -1) {
          ESP_LOGE(TAG, "Periodic MQTT publish failed.");
        }
      } else {
        ESP_LOGE(TAG, "Failed to create JSON status payload.");
      }
    } else {
      ESP_LOGW(TAG, "MQTT not connected, skipping periodic publish.");
    }

    vTaskDelay(pdMS_TO_TICKS(CONFIG_MQTT_PERIODIC_PUBLISH_INTERVAL_MS));
  }
}

/**
 * @brief Starts a task to handle a fall alert.
 * @param gps_data A pointer to the latest GPS data.
 */
esp_err_t sim4g_gps_start_fall_alert(const gps_data_t *gps_data) {
  gps_data_t *task_param = (gps_data_t *)malloc(sizeof(gps_data_t));
  if (task_param == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for fall alert task parameter.");
    return ESP_FAIL;
  }
  memcpy(task_param, gps_data, sizeof(gps_data_t));

  BaseType_t result =
      xTaskCreate(fall_alert_task, "fall_alert_task", ALERT_TASK_STACK_SIZE,
                  task_param, ALERT_TASK_PRIORITY, NULL);

  if (result != pdPASS) {
    ESP_LOGE(TAG, "Failed to create fall_alert_task");
    free(task_param);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Fall alert task created successfully");
  return ESP_OK;
}

/**
 * @brief Initializes the SIM4G GPS module and starts the monitoring task.
 */
/**
 * @brief Initializes the SIM4G GPS module and starts the monitoring task.
 */
esp_err_t sim4g_gps_init(void) {
  ESP_LOGI(TAG, "Initializing SIM4G AT module...");
  esp_err_t err = sim4g_at_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SIM4G AT initialization failed: %s", esp_err_to_name(err));
    return err;
  }

  // --- NEW: APN CONFIGURATION ---
  // Configure the APN based on the value from Kconfig
  ESP_LOGI(TAG, "Configuring APN: %s", CONFIG_SIM_APN);
  err = sim4g_at_configure_apn(CONFIG_SIM_APN);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "APN configuration failed: %s", esp_err_to_name(err));
    // NOTE: You may want to return an error here depending on if a working
    // APN is a hard requirement for your application.
  }
  // --- END NEW ---

  s_gps_at_mutex = xSemaphoreCreateMutex();
  if (s_gps_at_mutex == NULL) {
    ESP_LOGE(TAG, "Failed to create mutex");
    return ESP_FAIL;
  }

  xTaskCreate(mqtt_monitoring_task, "mqtt_mon_task", MQTT_TASK_STACK_SIZE, NULL,
              MQTT_TASK_PRIORITY, NULL);

  return ESP_OK;
}

void sim4g_gps_set_phone_number(const char *number) {
  if (number) {
    strncpy(s_phone_number, number, sizeof(s_phone_number) - 1);
    s_phone_number[sizeof(s_phone_number) - 1] = '\0';
    ESP_LOGI(TAG, "Phone number set to: %s", s_phone_number);
  }
}
