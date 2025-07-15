#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <stdlib.h>
#include <string.h>

#define AT_BUFFER_SIZE 256

static esp_err_t send_at_command(const char *cmd, char *resp, size_t len) {
  return comm_uart_send_command(cmd, resp, len);
}

esp_err_t sim4g_init(void) {
  char response[AT_BUFFER_SIZE] = {0};
  if (send_at_command("AT", response, sizeof(response)) != COMM_SUCCESS)
    return ESP_FAIL;

  if (!strstr(response, "OK")) {
    DEBUGS_LOGE("SIM module not responding.");
    return ESP_FAIL;
  }

  DEBUGS_LOGI("SIM module ready.");
  return ESP_OK;
}

esp_err_t sim4g_enable_gps(void) {
  char response[AT_BUFFER_SIZE] = {0};
  if (send_at_command("AT+QGPS=1", response, sizeof(response)) != COMM_SUCCESS)
    return ESP_FAIL;

  if (!strstr(response, "OK")) {
    DEBUGS_LOGE("Failed to start GPS.");
    return ESP_FAIL;
  }

  DEBUGS_LOGI("GPS enabled.");
  return ESP_OK;
}

esp_err_t sim4g_get_location(gps_location_t *loc) {
  if (!loc)
    return ESP_ERR_INVALID_ARG;

  char response[AT_BUFFER_SIZE] = {0};
  if (send_at_command("AT+QGPSLOC=1", response, sizeof(response)) !=
      COMM_SUCCESS)
    return ESP_FAIL;

  if (!strstr(response, "+QGPSLOC:")) {
    DEBUGS_LOGW("No GPS lock or invalid response.");
    loc->valid = false;
    return ESP_FAIL;
  }

  // Example response: +QGPSLOC: 123519.00,10.762622,106.660172,7.5,...
  // Parse manually
  char *start = strstr(response, "+QGPSLOC: ");
  if (!start)
    return ESP_FAIL;

  float lat = 0, lon = 0, alt = 0;
  int hh, mm, ss;

  if (sscanf(start, "+QGPSLOC: %2d%2d%2d.%*d,%f,%f,%f", &hh, &mm, &ss, &lat,
             &lon, &alt) < 6) {
    DEBUGS_LOGE("Failed to parse GPS data.");
    loc->valid = false;
    return ESP_FAIL;
  }

  loc->latitude = lat;
  loc->longitude = lon;
  loc->altitude = alt;
  loc->utc_hour = hh;
  loc->utc_min = mm;
  loc->utc_sec = ss;
  loc->valid = true;

  DEBUGS_LOGI("GPS: %.6f, %.6f, alt=%.2fm @ %02d:%02d:%02d", lat, lon, alt, hh,
              mm, ss);

  return ESP_OK;
}

esp_err_t sim4g_send_sms(const char *phone_number, const char *message) {
  char response[AT_BUFFER_SIZE] = {0};

  if (send_at_command("AT+CMGF=1", response, sizeof(response)) != COMM_SUCCESS)
    return ESP_FAIL;

  // Set recipient
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);
  if (send_at_command(cmd, response, sizeof(response)) != COMM_SUCCESS)
    return ESP_FAIL;

  // Send message content with CTRL+Z
  char full_message[180];
  snprintf(full_message, sizeof(full_message), "%s\x1A", message);
  if (send_at_command(full_message, response, sizeof(response)) != COMM_SUCCESS)
    return ESP_FAIL;

  if (!strstr(response, "OK")) {
    DEBUGS_LOGE("SMS send failed.");
    return ESP_FAIL;
  }

  DEBUGS_LOGI("SMS sent to %s", phone_number);
  return ESP_OK;
}

