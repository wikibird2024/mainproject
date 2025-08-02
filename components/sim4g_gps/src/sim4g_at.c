#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "debugs.h"
#include "sim4g_at.h"
#include "sim4g_at_cmd.h"

#define AT_RESPONSE_TIMEOUT_MS 200
#define AT_SMS_WAIT_MS 500
#define AT_GPSLOC_DELAY_MS 300
#define AT_SMS_RESPONSE_TIMEOUT 2000

bool sim4g_at_enable_gps(void) {
  char response[64] = {0};

  comm_uart_send_command(AT_CMD_GPS_AUTOSTART, NULL, 0);
  vTaskDelay(pdMS_TO_TICKS(AT_RESPONSE_TIMEOUT_MS));

  comm_uart_send_command(AT_CMD_GPS_ON, response, sizeof(response));
  bool success = strstr(response, "OK") != NULL;

  DEBUGS_LOGI("Enable GPS: %s", success ? "Success" : "Failed");
  return success;
}

bool sim4g_at_get_location(char *timestamp, char *lat, char *lon, size_t len) {
  char response[128] = {0};

  comm_uart_send_command(AT_CMD_GPS_LOC_QUERY, response, sizeof(response));
  vTaskDelay(pdMS_TO_TICKS(AT_GPSLOC_DELAY_MS));

  int ret = sscanf(response, "+QGPSLOC: %19[^,],%19[^,],%19[^,]", timestamp,
                   lat, lon);
  bool success = (ret == 3);

  DEBUGS_LOGI("Get location: %s", success ? "Parsed OK" : "Parse Failed");
  return success;
}

bool sim4g_at_send_sms(const char *phone, const char *message) {
  char cmd[64];
  char response[64];

  snprintf(cmd, sizeof(cmd), AT_CMD_SMS_SEND_FMT, phone);
  comm_uart_send_command(cmd, NULL, 0);
  vTaskDelay(pdMS_TO_TICKS(AT_SMS_WAIT_MS));

  comm_uart_send_command(message, NULL, 0);
  vTaskDelay(pdMS_TO_TICKS(AT_SMS_WAIT_MS));

  comm_uart_send_command(AT_CMD_SMS_CTRL_Z, response, sizeof(response));
  vTaskDelay(pdMS_TO_TICKS(AT_SMS_RESPONSE_TIMEOUT));

  bool success =
      strstr(response, "OK") != NULL || strstr(response, "+CMGS") != NULL;

  DEBUGS_LOGI("Send SMS to %s: %s", phone, success ? "Success" : "Failed");
  return success;
}
