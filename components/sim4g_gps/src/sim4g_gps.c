/**
 * @file sim4g_gps.c
 * @brief SIM4G GPS high-level API interface for initialization, location
 * tracking, and SMS alerts.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include "comm.h"         // UART command interface
#include "debugs.h"       // Logging macros
#include "sim4g_at.h"     // Internal AT command logic
#include "sim4g_at_cmd.h" // AT command definitions (optional, for clarity)
#include "sim4g_gps.h"    // Public API

#define TAG "SIM4G_GPS"
#define DEFAULT_PHONE CONFIG_SIM4G_DEFAULT_PHONE

// Internal State
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[16] = DEFAULT_PHONE;
static sim4g_gps_data_t last_location = {0};

void sim4g_gps_init(void) {
  if (!gps_mutex) {
    gps_mutex = xSemaphoreCreateMutex();
    if (!gps_mutex) {
      DEBUGS_LOGE("Failed to create GPS mutex");
      return;
    }
  }

  if (sim4g_at_enable_gps()) {
    DEBUGS_LOGI("GPS enabled successfully");
  } else {
    DEBUGS_LOGE("GPS initialization failed");
  }
}

void sim4g_gps_set_phone_number(const char *number) {
  if (number && strlen(number) < sizeof(phone_number)) {
    strncpy(phone_number, number, sizeof(phone_number));
    phone_number[sizeof(phone_number) - 1] = '\0';
    DEBUGS_LOGI("Phone number updated: %s", phone_number);
  } else {
    DEBUGS_LOGW("Invalid phone number input");
  }
}

bool sim4g_gps_is_enabled(void) {
  char response[64] = {0};
  if (!comm_uart_send_command(AT_CMD_GPS_CHECK, response, sizeof(response))) {
    DEBUGS_LOGW("GPS check failed");
    return false;
  }

  return strstr(response, "+QGPS: 1") != NULL;
}

bool sim4g_gps_update_location(sim4g_gps_data_t *out) {
  if (!out || !gps_mutex) {
    DEBUGS_LOGE("NULL param or GPS mutex not ready");
    return false;
  }

  bool success = false;
  if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    char ts[20], lat[20], lon[20];
    if (sim4g_at_get_location(ts, lat, lon, sizeof(ts))) {
      strncpy(out->timestamp, ts, sizeof(out->timestamp));
      strncpy(out->latitude, lat, sizeof(out->latitude));
      strncpy(out->longitude, lon, sizeof(out->longitude));
      out->valid = true;
      success = true;
    } else {
      memset(out, 0, sizeof(*out));
      out->valid = false;
      DEBUGS_LOGW("Failed to update GPS location");
    }
    xSemaphoreGive(gps_mutex);
  }

  return success;
}

sim4g_gps_data_t sim4g_gps_get_location(void) {
  memset(&last_location, 0, sizeof(last_location));
  last_location.valid = false;

  if (!gps_mutex)
    return last_location;

  if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    if (sim4g_at_get_location(last_location.timestamp, last_location.latitude,
                              last_location.longitude,
                              sizeof(last_location.timestamp))) {
      last_location.valid = true;
    } else {
      DEBUGS_LOGW("Could not get GPS location");
    }
    xSemaphoreGive(gps_mutex);
  }

  return last_location;
}

// Internal FreeRTOS task to send SMS
static void send_sms_task(void *param) {
  sim4g_gps_data_t *loc = (sim4g_gps_data_t *)param;
  if (!loc || !loc->valid) {
    DEBUGS_LOGE("Invalid data passed to SMS task");
    vTaskDelete(NULL);
    return;
  }

  char msg[256];
  snprintf(msg, sizeof(msg), "Fall detected!\nLat: %s\nLon: %s\nTime: %s",
           loc->latitude, loc->longitude, loc->timestamp);

  DEBUGS_LOGI("Sending SMS to %s: %s", phone_number, msg);
  sim4g_at_send_sms(phone_number, msg);

  vPortFree(loc);
  vTaskDelete(NULL);
}

void send_fall_alert_sms(const sim4g_gps_data_t *location) {
  if (!location || !location->valid)
    return;

  sim4g_gps_data_t *copy = pvPortMalloc(sizeof(sim4g_gps_data_t));
  if (!copy) {
    DEBUGS_LOGE("Out of memory when allocating SMS task");
    return;
  }

  *copy = *location;
  if (xTaskCreate(send_sms_task, "sms_task", 4096, copy, 5, NULL) != pdPASS) {
    DEBUGS_LOGE("Failed to create SMS task");
    vPortFree(copy);
  }
}
/**
 * @brief Wrapper function to match public API definition.
 *
 * Called from upper layers (e.g. fall detection).
 */
void sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *data,
                                     void (*callback)(bool success)) {
  (void)callback; // Hiện chưa dùng callback
  send_fall_alert_sms(data);
}
