#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "debugs.h"
#include "sim4g_at.h"
#include "sim4g_at_cmd.h"
#include "sim4g_gps.h"

#define DEFAULT_PHONE CONFIG_SIM4G_DEFAULT_PHONE

// Internal state
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[16] = DEFAULT_PHONE;
static sim4g_gps_data_t last_location = {0};

esp_err_t sim4g_gps_init(void) {
  if (!gps_mutex) {
    gps_mutex = xSemaphoreCreateMutex();
    if (!gps_mutex) {
      DEBUGS_LOGE("Failed to create GPS mutex");
      return ESP_ERR_NO_MEM;
    }
  }

  esp_err_t err = sim4g_at_enable_gps();
  if (err == ESP_OK) {
    DEBUGS_LOGI("GPS enabled successfully");
  } else {
    DEBUGS_LOGE("GPS initialization failed: %s", esp_err_to_name(err));
  }
  return err;
}

esp_err_t sim4g_gps_set_phone_number(const char *number) {
  if (!number || strlen(number) >= sizeof(phone_number)) {
    DEBUGS_LOGW("Invalid phone number input");
    return ESP_ERR_INVALID_ARG;
  }

  strncpy(phone_number, number, sizeof(phone_number));
  phone_number[sizeof(phone_number) - 1] = '\0';
  DEBUGS_LOGI("Phone number updated: %s", phone_number);
  return ESP_OK;
}

esp_err_t sim4g_gps_is_enabled(bool *enabled) {
  if (!enabled)
    return ESP_ERR_INVALID_ARG;

  char response[64] = {0};
  
  // FIXED: Use new AT command API instead of direct string
  esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_STATUS_ID, response, sizeof(response));
  if (err != ESP_OK) {
    DEBUGS_LOGW("GPS status query failed: %s", esp_err_to_name(err));
    return err;
  }

  *enabled = (strstr(response, "+QGPS: 1") != NULL);
  return ESP_OK;
}

esp_err_t sim4g_gps_update_location(sim4g_gps_data_t *out) {
  if (!out || !gps_mutex)
    return ESP_ERR_INVALID_ARG;

  esp_err_t result = ESP_FAIL;

  if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    char ts[20], lat[20], lon[20];
    
    // FIXED: Updated function signature - removed 'len' parameter
    esp_err_t err = sim4g_at_get_location(ts, lat, lon, sizeof(ts));
    if (err == ESP_OK) {
      strncpy(out->timestamp, ts, sizeof(out->timestamp) - 1);
      strncpy(out->latitude, lat, sizeof(out->latitude) - 1);
      strncpy(out->longitude, lon, sizeof(out->longitude) - 1);
      
      // Ensure null termination
      out->timestamp[sizeof(out->timestamp) - 1] = '\0';
      out->latitude[sizeof(out->latitude) - 1] = '\0';
      out->longitude[sizeof(out->longitude) - 1] = '\0';
      
      out->valid = true;
      result = ESP_OK;
    } else {
      memset(out, 0, sizeof(*out));
      out->valid = false;
      DEBUGS_LOGW("Failed to update GPS location: %s", esp_err_to_name(err));
    }
    xSemaphoreGive(gps_mutex);
  } else {
    DEBUGS_LOGW("Could not acquire GPS mutex");
    result = ESP_ERR_TIMEOUT;
  }

  return result;
}

sim4g_gps_data_t sim4g_gps_get_location(void) {
  memset(&last_location, 0, sizeof(last_location));
  last_location.valid = false;

  if (!gps_mutex)
    return last_location;

  if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    // FIXED: Updated function signature - removed 'len' parameter
    esp_err_t err = sim4g_at_get_location(
        last_location.timestamp, last_location.latitude,
        last_location.longitude, sizeof(last_location.timestamp));

    if (err == ESP_OK) {
      // Ensure null termination
      last_location.timestamp[sizeof(last_location.timestamp) - 1] = '\0';
      last_location.latitude[sizeof(last_location.latitude) - 1] = '\0';
      last_location.longitude[sizeof(last_location.longitude) - 1] = '\0';
      
      last_location.valid = true;
    } else {
      DEBUGS_LOGW("Could not get GPS location: %s", esp_err_to_name(err));
    }
    xSemaphoreGive(gps_mutex);
  }

  return last_location;
}

// Internal FreeRTOS SMS task
static void send_sms_task(void *param) {
  sim4g_gps_data_t *loc = (sim4g_gps_data_t *)param;

  if (!loc || !loc->valid) {
    DEBUGS_LOGE("Invalid GPS data for SMS task");
    if (loc) vPortFree(loc);
    vTaskDelete(NULL);
    return;
  }

  char msg[256];
  snprintf(msg, sizeof(msg), "Fall detected!\nLat: %s\nLon: %s\nTime: %s",
           loc->latitude, loc->longitude, loc->timestamp);

  DEBUGS_LOGI("Sending SMS to %s:\n%s", phone_number, msg);

  esp_err_t err = sim4g_at_send_sms(phone_number, msg);
  if (err != ESP_OK) {
    DEBUGS_LOGE("SMS send failed: %s", esp_err_to_name(err));
  } else {
    DEBUGS_LOGI("SMS sent successfully");
  }

  vPortFree(loc);
  vTaskDelete(NULL);
}

esp_err_t sim4g_gps_send_fall_alert_sms(const sim4g_gps_data_t *location) {
  if (!location || !location->valid)
    return ESP_ERR_INVALID_ARG;

  char msg[256];
  snprintf(msg, sizeof(msg), "Fall detected!\nLat: %s\nLon: %s\nTime: %s",
           location->latitude, location->longitude, location->timestamp);

  DEBUGS_LOGI("Sending SMS to %s:\n%s", phone_number, msg);

  return sim4g_at_send_sms(phone_number, msg);
}

esp_err_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *data,
                                          void (*callback)(bool success)) {
  if (!data || !data->valid) {
    DEBUGS_LOGE("Invalid GPS data for SMS alert");
    return ESP_ERR_INVALID_ARG;
  }

  sim4g_gps_data_t *copy = pvPortMalloc(sizeof(sim4g_gps_data_t));
  if (!copy) {
    DEBUGS_LOGE("Out of memory for SMS task allocation");
    return ESP_ERR_NO_MEM;
  }

  *copy = *data;

  // Optional: bundle callback if needed in future (not used now)
  if (xTaskCreate(send_sms_task, "sim4g_sms_task", 4096, copy, 5, NULL) !=
      pdPASS) {
    DEBUGS_LOGE("Failed to create SMS task");
    vPortFree(copy);
    return ESP_FAIL;
  }

  return ESP_OK;
}
