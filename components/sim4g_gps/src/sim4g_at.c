#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <stdio.h>

#include "comm.h"
#include "debugs.h"
#include "sim4g_at.h"
#include "sim4g_at_cmd.h"

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
//  GPS and SMS Functions
// -----------------------------------------------------------
esp_err_t sim4g_at_enable_gps(void) {
    char resp[64] = {0};

    // Optional: enable autogps feature
    // Using the new function
    sim4g_at_send_by_id(AT_CMD_GPS_AUTOGPS_ON_ID, NULL, 0);

    // Using the new function
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_ENABLE_ID, resp, sizeof(resp));
    if (err != ESP_OK || !strstr(resp, "OK")) {
        DEBUGS_LOGE("Enable GPS failed: %s", resp);
        return ESP_FAIL;
    }

    DEBUGS_LOGI("GPS enabled");
    return ESP_OK;
}

esp_err_t sim4g_at_get_location(char *timestamp, char *lat, char *lon,
                                size_t len) {
    if (!timestamp || !lat || !lon)
        return ESP_ERR_INVALID_ARG;

    char response[128] = {0};

    // Using the new function
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_LOCATION_ID, response, sizeof(response));
    if (err != ESP_OK)
        return err;

    int ret = sscanf(response, "+QGPSLOC: %19[^,],%19[^,],%19[^,]", timestamp,
                     lat, lon);
    if (ret != 3) {
        DEBUGS_LOGW("Failed to parse GPS: %s", response);
        return ESP_FAIL;
    }

    DEBUGS_LOGI("GPS OK: Time=%s Lat=%s Lon=%s", timestamp, lat, lon);
    return ESP_OK;
}

esp_err_t sim4g_at_send_sms(const char *phone, const char *message) {
    if (!phone || !message)
        return ESP_ERR_INVALID_ARG;

    char cmd[64];
    char response[128];
    const char *sms_prefix = at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].cmd_string;
    const uint32_t sms_timeout = at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].timeout_ms;
    const char *sms_end = at_command_table[AT_CMD_SMS_CTRL_Z_ID].cmd_string;

    // Construct and send: AT+CMGS=""
    snprintf(cmd, sizeof(cmd), "%s%s\"\r", sms_prefix, phone);
    comm_uart_send_command(cmd, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(sms_timeout));

    // Send the SMS body
    comm_uart_send_command(message, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(sms_timeout));

    // Send Ctrl+Z (end of message)
    memset(response, 0, sizeof(response));
    comm_uart_send_command(sms_end, response, sizeof(response));
    vTaskDelay(pdMS_TO_TICKS(at_command_table[AT_CMD_SMS_CTRL_Z_ID].timeout_ms));
    
    if (strstr(response, "OK") || strstr(response, "+CMGS")) {
        DEBUGS_LOGI("SMS sent to %s", phone);
        return ESP_OK;
    }

    DEBUGS_LOGW("SMS failed: %s", response);
    return ESP_FAIL;
}
