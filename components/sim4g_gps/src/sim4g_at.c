// File: components/sim4g_gps/src/sim4g_at.c

#include <string.h> //
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sim4g_at.h"
#include "comm.h"
#include "sdkconfig.h" // 

static const char *TAG = "SIM4G_AT";

// AT Command table with their corresponding ID, command string, and timeout
const at_command_t at_command_table[AT_CMD_MAX_COUNT] = {
    [AT_CMD_TEST_ID]                = {AT_CMD_TEST_ID, "AT\r\n", 300},
    [AT_CMD_GET_FIRMWARE_ID]        = {AT_CMD_GET_FIRMWARE_ID, "ATI\r\n", 500},
    [AT_CMD_SEND_SMS_PREFIX_ID]     = {AT_CMD_SEND_SMS_PREFIX_ID, "AT+CMGS=\"", 500},
    [AT_CMD_SMS_CTRL_Z_ID]          = {AT_CMD_SMS_CTRL_Z_ID, "\x1A", 5000},
    [AT_CMD_SMS_MODE_TEXT_ID]       = {AT_CMD_SMS_MODE_TEXT_ID, "AT+CMGF=1\r\n", 500},
    [AT_CMD_SET_CHARSET_GSM_ID]     = {AT_CMD_SET_CHARSET_GSM_ID, "AT+CSCS=\"GSM\"\r\n", 500},
    [AT_CMD_REGISTRATION_STATUS_ID] = {AT_CMD_REGISTRATION_STATUS_ID, "AT+CREG?\r\n", 500},
    
    [AT_CMD_GPS_ENABLE_ID]          = {AT_CMD_GPS_ENABLE_ID, "AT+QGPS=1\r\n", 5000},
    [AT_CMD_GPS_DISABLE_ID]         = {AT_CMD_GPS_DISABLE_ID, "AT+QGPSEND\r\n", 500},
    [AT_CMD_GPS_STATUS_ID]          = {AT_CMD_GPS_STATUS_ID, "AT+QGPS?\r\n", 500},
    [AT_CMD_GPS_LOCATION_ID]        = {AT_CMD_GPS_LOCATION_ID, "AT+QGPSLOC=2\r\n", 300},
    
    [AT_CMD_GPS_AUTOGPS_ON_ID]      = {AT_CMD_GPS_AUTOGPS_ON_ID, "AT+QGPSCFG=\"autogps\",1\r\n", 500},
    [AT_CMD_GPS_OUTPORT_USB_ID]     = {AT_CMD_GPS_OUTPORT_USB_ID, "AT+QGPSCFG=\"outport\",\"usb\"\r\n", 500},
    [AT_CMD_GPS_XTRA_ENABLE_ID]     = {AT_CMD_GPS_XTRA_ENABLE_ID, "AT+QGPSXTRA=1\r\n", 500},
    [AT_CMD_GPS_UTC_TIME_ID]        = {AT_CMD_GPS_UTC_TIME_ID, "AT+QGPSTIME\r\n", 500},
    [AT_CMD_CELL_LOCATE_ID]         = {AT_CMD_CELL_LOCATE_ID, "AT+CLBS=1\r\n", 500},
};


esp_err_t sim4g_at_send_by_id(at_cmd_id_t cmd_id, char *response, size_t len) {
    if (cmd_id >= AT_CMD_MAX_COUNT) {
        ESP_LOGE(TAG, "Invalid AT command ID: %d", cmd_id);
        return ESP_ERR_INVALID_ARG;
    }

    const at_command_t *cmd = &at_command_table[cmd_id];
    ESP_LOGI(TAG, "Sending AT command: %s", cmd->cmd_string);

    // Sửa lỗi: comm_uart_send_command trả về comm_result_t, cần kiểm tra với COMM_SUCCESS
    comm_result_t res = comm_uart_send_command(cmd->cmd_string, response, len, cmd->timeout_ms);
    if (res == COMM_SUCCESS) {
        ESP_LOGI(TAG, "Received response: %s", response);
        return ESP_OK; // Chuyển đổi thành ESP_OK để giữ sự nhất quán của hàm
    } else {
        ESP_LOGW(TAG, "Command failed with result: %d", res);
        return ESP_FAIL; // Chuyển đổi thành ESP_FAIL
    }
}


esp_err_t sim4g_at_configure_gps(void) {
    char resp[64] = {0};
    
    ESP_LOGI(TAG, "Attempting to configure GPS with autogps...");
    
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_AUTOGPS_ON_ID, resp, sizeof(resp));
    if (err != ESP_OK || !strstr(resp, "OK")) {
        ESP_LOGW(TAG, "GPS autogps configuration failed, but continuing: %s", resp);
        return ESP_FAIL;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return ESP_OK;
}

esp_err_t sim4g_at_check_network_registration(void) {
    char resp[64] = {0};
    
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_REGISTRATION_STATUS_ID, resp, sizeof(resp));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Network registration query failed.");
        return err;
    }

    if (strstr(resp, "+CREG: 0,1") != NULL || strstr(resp, "+CREG: 0,5") != NULL) {
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

esp_err_t sim4g_at_get_location(char *timestamp, char *lat, char *lon) {
    if (!timestamp || !lat || !lon) {
        return ESP_ERR_INVALID_ARG;
    }

    char resp[128] = {0};
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_LOCATION_ID, resp, sizeof(resp));

    if (err != ESP_OK || !strstr(resp, "+QGPSLOC:")) {
        ESP_LOGW(TAG, "Failed to get location. Response: %s", resp);
        return ESP_FAIL;
    }

    sscanf(resp, "+QGPSLOC: %*s,%[^,],%[^,],%[^,]", timestamp, lat, lon);
    return ESP_OK;
}

esp_err_t sim4g_at_send_sms(const char *phone, const char *message) {
    if (!phone || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    char cmd[64];
    char response[128] = {0};

    ESP_LOGI(TAG, "Attempting to send SMS to: %s", phone);

    // Step 1: Set to text mode
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_SMS_MODE_TEXT_ID, response, sizeof(response));
    if (err != ESP_OK || !strstr(response, "OK")) {
        ESP_LOGW(TAG, "Failed to set SMS text mode: %s", response);
        return ESP_FAIL;
    }

    // Step 2: Send the SMS prefix and phone number
    snprintf(cmd, sizeof(cmd), "%s%s\"\r", at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].cmd_string, phone);
    
    // Sửa lỗi: Cần kiểm tra giá trị trả về và truyền tham số timeout
    comm_result_t res = comm_uart_send_command(cmd, response, sizeof(response), at_command_table[AT_CMD_SEND_SMS_PREFIX_ID].timeout_ms);
    if (res != COMM_SUCCESS || !strstr(response, ">")) {
        ESP_LOGW(TAG, "Failed to get SMS prompt: %s", response);
        return ESP_FAIL;
    }
    
    // Step 3: Send the SMS body and Ctrl+Z
    snprintf(cmd, sizeof(cmd), "%s\x1A", message);

    // Sửa lỗi: Cần kiểm tra giá trị trả về và truyền tham số timeout
    res = comm_uart_send_command(cmd, response, sizeof(response), at_command_table[AT_CMD_SMS_CTRL_Z_ID].timeout_ms);
    
    if (res == COMM_SUCCESS && (strstr(response, "OK") || strstr(response, "+CMGS"))) {
        ESP_LOGI(TAG, "SMS sent to %s", phone);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "SMS failed: %s", response);
    return ESP_FAIL;
}


esp_err_t sim4g_at_init(void) {
    ESP_LOGI(TAG, "Initializing SIM4G AT driver...");
    
    // Sửa lỗi: comm_uart_init giờ nhận 3 tham số
    esp_err_t err = comm_uart_init(CONFIG_COMM_UART_PORT_NUM, CONFIG_COMM_UART_TX_PIN, CONFIG_COMM_UART_RX_PIN);
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
