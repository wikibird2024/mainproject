#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "comm.h"
#include "sim4g_at.h"
#include "sim4g_at_cmd.h"

static const char *TAG = "SIM4G_AT";

// -----------------------------------------------------------
// AT Command Table Definition
// -----------------------------------------------------------
const at_command_t at_command_table[AT_CMD_MAX_COUNT] = {
    [AT_CMD_TEST_ID]                = {AT_CMD_TEST_ID, "AT\r\n", 200},
    [AT_CMD_ECHO_OFF_ID]            = {AT_CMD_ECHO_OFF_ID, "ATE0\r\n", 200},
    [AT_CMD_SAVE_CFG_ID]            = {AT_CMD_SAVE_CFG_ID, "AT&W\r\n", 500},
    [AT_CMD_GET_MODULE_INFO_ID]     = {AT_CMD_GET_MODULE_INFO_ID, "ATI\r\n", 500},
    [AT_CMD_GET_IMEI_ID]            = {AT_CMD_GET_IMEI_ID, "AT+CGSN\r\n", 500},

    [AT_CMD_CHECK_SIM_ID]           = {AT_CMD_CHECK_SIM_ID, "AT+CPIN?\r\n", 500},
    [AT_CMD_GET_IMSI_ID]            = {AT_CMD_GET_IMSI_ID, "AT+CIMI\r\n", 500},
    [AT_CMD_GET_ICCID_ID]           = {AT_CMD_GET_ICCID_ID, "AT+QCCID\r\n", 500},
    [AT_CMD_GET_SMS_MODE_ID]        = {AT_CMD_GET_SMS_MODE_ID, "AT+CMGF?\r\n", 500},
    [AT_CMD_GET_CHARSET_ID]         = {AT_CMD_GET_CHARSET_ID, "AT+CSCS?\r\n", 500},

    [AT_CMD_SIGNAL_QUALITY_ID]      = {AT_CMD_SIGNAL_QUALITY_ID, "AT+CSQ\r\n", 500},
    [AT_CMD_GET_OPERATOR_ID]        = {AT_CMD_GET_OPERATOR_ID, "AT+COPS?\r\n", 500},
    [AT_CMD_GET_NETWORK_TYPE_ID]    = {AT_CMD_GET_NETWORK_TYPE_ID, "AT+QNWINFO\r\n", 500},
    [AT_CMD_ATTACH_STATUS_ID]       = {AT_CMD_ATTACH_STATUS_ID, "AT+CGATT?\r\n", 500},
    [AT_CMD_REGISTRATION_STATUS_ID] = {AT_CMD_REGISTRATION_STATUS_ID, "AT+CREG?\r\n", 500},

    [AT_CMD_SMS_MODE_TEXT_ID]       = {AT_CMD_SMS_MODE_TEXT_ID, "AT+CMGF=1\r\n", 500},
    [AT_CMD_SET_CHARSET_GSM_ID]     = {AT_CMD_SET_CHARSET_GSM_ID, "AT+CSCS=\"GSM\"\r\n", 500},
    [AT_CMD_SEND_SMS_PREFIX_ID]     = {AT_CMD_SEND_SMS_PREFIX_ID, "AT+CMGS=\"", 500},
    [AT_CMD_SMS_CTRL_Z_ID]          = {AT_CMD_SMS_CTRL_Z_ID, "\x1A", 2000},

    [AT_CMD_ANSWER_CALL_ID]         = {AT_CMD_ANSWER_CALL_ID, "ATA\r\n", 500},
    [AT_CMD_HANGUP_CALL_ID]         = {AT_CMD_HANGUP_CALL_ID, "ATH\r\n", 500},

    [AT_CMD_GPS_ENABLE_ID]          = {AT_CMD_GPS_ENABLE_ID, "AT+QGPS=1\r\n", 500},
    [AT_CMD_GPS_DISABLE_ID]         = {AT_CMD_GPS_DISABLE_ID, "AT+QGPSEND\r\n", 500},
    [AT_CMD_GPS_STATUS_ID]          = {AT_CMD_GPS_STATUS_ID, "AT+QGPS?\r\n", 500},
    [AT_CMD_GPS_LOCATION_ID]        = {AT_CMD_GPS_LOCATION_ID, "AT+QGPSLOC=2\r\n", 300},
    
    [AT_CMD_GPS_AUTOGPS_ON_ID]      = {AT_CMD_GPS_AUTOGPS_ON_ID, "AT+QGPSCFG=\"autogps\",1\r\n", 500},
    [AT_CMD_GPS_OUTPORT_USB_ID]     = {AT_CMD_GPS_OUTPORT_USB_ID, "AT+QGPSCFG=\"outport\",\"usb\"\r\n", 500},
    [AT_CMD_GPS_XTRA_ENABLE_ID]     = {AT_CMD_GPS_XTRA_ENABLE_ID, "AT+QGPSXTRA=1\r\n", 500},
    [AT_CMD_GPS_UTC_TIME_ID]        = {AT_CMD_GPS_UTC_TIME_ID, "AT+QGPSTIME\r\n", 500},

    [AT_CMD_CELL_LOCATE_ID]         = {AT_CMD_CELL_LOCATE_ID, "AT+CLBS=1\r\n", 500},
};

// -----------------------------------------------------------
// Refactored Core Send Function
// -----------------------------------------------------------
esp_err_t sim4g_at_send_by_id(at_cmd_id_t cmd_id, char *response, size_t len) {
    if (cmd_id >= AT_CMD_MAX_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    const at_command_t *cmd_entry = &at_command_table[cmd_id];
    const char *cmd_string = cmd_entry->cmd_string;
    uint32_t timeout = cmd_entry->timeout_ms;

    if (!cmd_string) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Clear the response buffer if provided
    if (response) {
        memset(response, 0, len);
    }
    
    comm_uart_send_command(cmd_string, response, len);
    vTaskDelay(pdMS_TO_TICKS(timeout));

    if (response && strlen(response) == 0) {
        return ESP_ERR_TIMEOUT;
    }

    if (response && strstr(response, "ERROR")) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

// -----------------------------------------------------------
//  Wrapper Functions for Specific Commands
// -----------------------------------------------------------

esp_err_t sim4g_at_enable_gps(void) {
    char resp[64] = {0};

    // Use the core send function with the correct AT command ID
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_ENABLE_ID, resp, sizeof(resp));
    if (err != ESP_OK || !strstr(resp, "OK")) {
        ESP_LOGE(TAG, "Enable GPS failed: %s", resp);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "GPS enabled");
    return ESP_OK;
}

esp_err_t sim4g_at_get_location(char *timestamp, char *lat, char *lon) {
    if (!timestamp || !lat || !lon) {
        return ESP_ERR_INVALID_ARG;
    }

    char response[128] = {0};

    // Use the core send function to get the raw GPS data
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_LOCATION_ID, response, sizeof(response));
    if (err != ESP_OK) {
        return err;
    }

    // Use sscanf to parse the raw data into the output buffers
    int ret = sscanf(response, "+QGPSLOC: %19[^,],%19[^,],%19[^,]", timestamp, lat, lon);
    if (ret != 3) {
        ESP_LOGW(TAG, "Failed to parse GPS: %s", response);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "GPS OK: Time=%s Lat=%s Lon=%s", timestamp, lat, lon);
    return ESP_OK;
}

esp_err_t sim4g_at_send_sms(const char *phone, const char *message) {
    if (!phone || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    char cmd[64];
    char response[128] = {0};

    // Step 1: Send the SMS prefix and phone number
    snprintf(cmd, sizeof(cmd), "%s%s\"\r", at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].cmd_string, phone);
    comm_uart_send_command(cmd, response, sizeof(response));

    // Check for the '>' prompt from the module
    if (!strstr(response, ">")) {
        ESP_LOGW(TAG, "Failed to get SMS prompt: %s", response);
        return ESP_FAIL;
    }
    
    // Step 2: Send the SMS body
    comm_uart_send_command(message, response, sizeof(response));

    // Step 3: Send Ctrl+Z to end the message
    comm_uart_send_command(at_command_table[AT_CMD_SMS_CTRL_Z_ID].cmd_string, response, sizeof(response));
    vTaskDelay(pdMS_TO_TICKS(at_command_table[AT_CMD_SMS_CTRL_Z_ID].timeout_ms));
    
    if (strstr(response, "OK") || strstr(response, "+CMGS")) {
        ESP_LOGI(TAG, "SMS sent to %s", phone);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "SMS failed: %s", response);
    return ESP_FAIL;
}
