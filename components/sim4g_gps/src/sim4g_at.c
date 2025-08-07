// t File: components/sim4g_gps/src/sim4g_at.c

#include "sim4g_at.h"
#include "comm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "SIM4G_AT";

// AT Command table
const at_command_t at_command_table[AT_CMD_MAX_COUNT] = {
    [AT_CMD_TEST_ID] = {AT_CMD_TEST_ID, "AT\r\n", 300},
    [AT_CMD_GET_FIRMWARE_ID] = {AT_CMD_GET_FIRMWARE_ID, "ATI\r\n", 500},
    [AT_CMD_SEND_SMS_PREFIX_ID] = {AT_CMD_SEND_SMS_PREFIX_ID, "AT+CMGS=\"",
                                   500},
    [AT_CMD_SMS_CTRL_Z_ID] = {AT_CMD_SMS_CTRL_Z_ID, "\x1A", 5000},
    [AT_CMD_SMS_MODE_TEXT_ID] = {AT_CMD_SMS_MODE_TEXT_ID, "AT+CMGF=1\r\n", 500},
    [AT_CMD_SET_CHARSET_GSM_ID] = {AT_CMD_SET_CHARSET_GSM_ID,
                                   "AT+CSCS=\"GSM\"\r\n", 500},
    [AT_CMD_REGISTRATION_STATUS_ID] = {AT_CMD_REGISTRATION_STATUS_ID,
                                       "AT+CREG?\r\n", 500},

    [AT_CMD_GPS_ENABLE_ID] = {AT_CMD_GPS_ENABLE_ID, "AT+QGPS=1\r\n", 5000},
    [AT_CMD_GPS_DISABLE_ID] = {AT_CMD_GPS_DISABLE_ID, "AT+QGPSEND\r\n", 500},
    [AT_CMD_GPS_STATUS_ID] = {AT_CMD_GPS_STATUS_ID, "AT+QGPS?\r\n", 500},
    [AT_CMD_GPS_LOCATION_ID] = {AT_CMD_GPS_LOCATION_ID, "AT+QGPSLOC=2\r\n",
                                300}, // Using your original AT command

    [AT_CMD_GPS_AUTOGPS_ON_ID] = {AT_CMD_GPS_AUTOGPS_ON_ID,
                                  "AT+QGPSCFG=\"autogps\",1\r\n", 500},
    [AT_CMD_GPS_OUTPORT_USB_ID] = {AT_CMD_GPS_OUTPORT_USB_ID,
                                   "AT+QGPSCFG=\"outport\",\"usb\"\r\n", 500},
    [AT_CMD_GPS_XTRA_ENABLE_ID] = {AT_CMD_GPS_XTRA_ENABLE_ID,
                                   "AT+QGPSXTRA=1\r\n", 500},
    [AT_CMD_GPS_UTC_TIME_ID] = {AT_CMD_GPS_UTC_TIME_ID, "AT+QGPSTIME\r\n", 500},
    [AT_CMD_CELL_LOCATE_ID] = {AT_CMD_CELL_LOCATE_ID, "AT+CLBS=1\r\n", 500},

    [AT_CMD_SET_APN_ID] = {AT_CMD_SET_APN_ID, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n",
                           5000},
};

esp_err_t sim4g_at_send_by_id(at_cmd_id_t cmd_id, char *response, size_t len) {
  if (cmd_id >= AT_CMD_MAX_COUNT) {
    ESP_LOGE(TAG, "Invalid AT command ID: %d", cmd_id);
    return ESP_ERR_INVALID_ARG;
  }

  const at_command_t *cmd = &at_command_table[cmd_id];
  ESP_LOGI(TAG, "Sending AT command: %s", cmd->cmd_string);

  comm_result_t res =
      comm_uart_send_command(cmd->cmd_string, response, len, cmd->timeout_ms);
  if (res == COMM_SUCCESS) {
    ESP_LOGI(TAG, "Received response: %s", response);
    return ESP_OK;
  } else {
    ESP_LOGW(TAG, "Command failed with result: %d", res);
    return ESP_FAIL;
  }
}

esp_err_t sim4g_at_configure_gps(void) {
  char resp[64] = {0};

  ESP_LOGI(TAG, "Attempting to configure GPS with autogps...");

  esp_err_t err =
      sim4g_at_send_by_id(AT_CMD_GPS_AUTOGPS_ON_ID, resp, sizeof(resp));
  if (err != ESP_OK || !strstr(resp, "OK")) {
    ESP_LOGW(TAG, "GPS autogps configuration failed: %s", resp);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t sim4g_at_configure_apn(const char *apn) {
  if (!apn) {
    return ESP_ERR_INVALID_ARG;
  }

  char command[128];
  snprintf(command, sizeof(command),
           at_command_table[AT_CMD_SET_APN_ID].cmd_string, apn);

  char resp[64] = {0};
  comm_result_t res =
      comm_uart_send_command(command, resp, sizeof(resp),
                             at_command_table[AT_CMD_SET_APN_ID].timeout_ms);

  if (res == COMM_SUCCESS && strstr(resp, "OK")) {
    ESP_LOGI(TAG, "APN configured successfully.");
    return ESP_OK;
  }

  ESP_LOGE(TAG, "Failed to configure APN. Response: %s", resp);
  return ESP_FAIL;
}

esp_err_t sim4g_at_check_network_registration(void) {
  char resp[64] = {0};

  esp_err_t err =
      sim4g_at_send_by_id(AT_CMD_REGISTRATION_STATUS_ID, resp, sizeof(resp));
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Network registration query failed.");
    return err;
  }

  if (strstr(resp, "+CREG: 0,1") || strstr(resp, "+CREG: 0,5")) {
    ESP_LOGI(TAG, "Network registered successfully.");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Network not registered yet. Response: %s", resp);
  return ESP_FAIL;
}

esp_err_t sim4g_at_enable_gps(void) {
  char resp[64] = {0};

  ESP_LOGI(TAG, "Attempting to enable GPS...");

  esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_ENABLE_ID, resp, sizeof(resp));
  if (err != ESP_OK || !strstr(resp, "OK")) {
    ESP_LOGE(TAG, "Enable GPS failed. Module response: %s", resp);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "GPS enabled");
  return ESP_OK;
}

esp_err_t sim4g_at_get_gps(gps_data_t *gps_data) {
  if (!gps_data) {
    return ESP_ERR_INVALID_ARG;
  }

  char resp[256] = {0};
  esp_err_t err =
      sim4g_at_send_by_id(AT_CMD_GPS_LOCATION_ID, resp, sizeof(resp));
  if (err != ESP_OK || !strstr(resp, "+QGPSLOC:")) {
    ESP_LOGW(TAG, "Failed to get GPS location. Response: %s", resp);
    memset(gps_data, 0, sizeof(gps_data_t));
    gps_data->has_gps_fix = false;
    return ESP_FAIL;
  }

  // This sscanf format is based on your original AT+QGPSLOC=2 command,
  // which returns a different format than the one I used before.
  char timestamp_str[32];
  if (sscanf(resp,
             "+QGPSLOC: %*[^,],%f,%f,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%31s",
             &gps_data->latitude, &gps_data->longitude, timestamp_str) == 3) {
    strncpy(gps_data->timestamp, timestamp_str,
            sizeof(gps_data->timestamp) - 1);
    gps_data->timestamp[sizeof(gps_data->timestamp) - 1] = '\0';

    gps_data->has_gps_fix = true;
    ESP_LOGI(TAG, "GPS fix acquired. Lat: %.6f, Lon: %.6f, Time: %s",
             gps_data->latitude, gps_data->longitude, gps_data->timestamp);
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "Failed to parse GPS location response: %s", resp);
    memset(gps_data, 0, sizeof(gps_data_t));
    gps_data->has_gps_fix = false;
    return ESP_FAIL;
  }
}

esp_err_t sim4g_at_send_sms(const char *phone, const char *message) {
  if (!phone || !message) {
    return ESP_ERR_INVALID_ARG;
  }

  char cmd[64];
  char response[128] = {0};

  ESP_LOGI(TAG, "Attempting to send SMS to: %s", phone);

  esp_err_t err =
      sim4g_at_send_by_id(AT_CMD_SMS_MODE_TEXT_ID, response, sizeof(response));
  if (err != ESP_OK || !strstr(response, "OK")) {
    ESP_LOGW(TAG, "Failed to set SMS text mode: %s", response);
    return ESP_FAIL;
  }

  snprintf(cmd, sizeof(cmd), "%s%s\"\r",
           at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].cmd_string, phone);
  comm_result_t res = comm_uart_send_command(
      cmd, response, sizeof(response),
      at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].timeout_ms);
  if (res != COMM_SUCCESS || !strstr(response, ">")) {
    ESP_LOGW(TAG, "Failed to get SMS prompt: %s", response);
    return ESP_FAIL;
  }

  snprintf(cmd, sizeof(cmd), "%s\x1A", message);
  res =
      comm_uart_send_command(cmd, response, sizeof(response),
                             at_command_table[AT_CMD_SMS_CTRL_Z_ID].timeout_ms);

  if (res == COMM_SUCCESS && strstr(response, "+CMGS")) {
    ESP_LOGI(TAG, "SMS sent to %s", phone);
    return ESP_OK;
  }

  ESP_LOGW(TAG, "SMS failed: %s", response);
  return ESP_FAIL;
}

esp_err_t sim4g_at_init(void) {
  ESP_LOGI(TAG, "Initializing SIM4G AT driver...");

  esp_err_t err =
      comm_uart_init(CONFIG_COMM_UART_PORT_NUM, CONFIG_COMM_UART_TX_PIN,
                     CONFIG_COMM_UART_RX_PIN);
  if (err != ESP_OK) {
    return err;
  }

  ESP_LOGI(TAG, "Sending test AT command...");
  char resp[64] = {0};

  err = sim4g_at_send_by_id(AT_CMD_TEST_ID, resp, sizeof(resp));

  if (err == ESP_OK && strstr(resp, "OK")) {
    ESP_LOGI(TAG, "SIM4G AT driver initialized successfully.");
    return ESP_OK;
  }

  ESP_LOGE(TAG, "SIM4G AT driver initialization failed.");
  return ESP_FAIL;
}
