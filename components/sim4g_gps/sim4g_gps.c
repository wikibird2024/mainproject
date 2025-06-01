#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdatomic.h>

#define TAG "SIM4G_GPS"

// Configuration constants
#define DEFAULT_PHONE_NUMBER "+84901234567"
#define COLD_START_TIME_MS 60000
#define NORMAL_OPERATION_TIME_MS 10000
#define NETWORK_CHECK_TIME_MS 5000
#define NETWORK_CHECK_INTERVAL_MS 1000
#define MIN_SIGNAL_STRENGTH 5
#define MAX_SMS_RETRY_COUNT 3
#define RESPONSE_BUFFER_SIZE 256
#define RETRY_DELAY_MS 500
#define SMS_BUFFER_SIZE 200
#define MUTEX_TIMEOUT_MS 1000
#define GPS_LOCATION_RETRY_INTERVAL_MS 1000
#define SMS_RETRY_INTERVAL_MS 1200
#define SMS_CONFIG_DELAY_MS 200
#define SMS_RECIPIENT_DELAY_MS 300
#define SMS_MESSAGE_DELAY_MS 200
#define SMS_POST_SEND_DELAY_MS 1200
#define SMS_TOTAL_TIMEOUT_MS 15000 // Timeout tổng quát: 15 giây
#define LOCATION_VALIDITY_MS 300000
#define MAX_SMS_TASKS 2
#define GPS_COORD_MIN_LENGTH 8
#define GPS_COORD_MAX_LENGTH 12
#define SMS_TASK_STACK_SIZE 4096
#define SMS_TASK_PRIORITY 5

// AT Commands
#define AT_CMD_GPS_ENABLE "AT+QGPS=1"
#define AT_CMD_GPS_LOCATION "AT+QGPSLOC?"
#define AT_CMD_NETWORK_REG "AT+CREG?"
#define AT_CMD_SIGNAL_STRENGTH "AT+CSQ"
#define AT_CMD_SMS_TEXT_MODE "AT+CMGF=1"
#define AT_CMD_SMS_SEND_HEADER "AT+CMGS=\"%s\""
#define AT_CMD_SMS_END "\x1A"
#define AT_CMD_GPS_AUTOSTART "AT+QGPSCFG=\"autogps\",1"

// Helper macros
#define DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define LOCK_MUTEX() (xSemaphoreTake(gps_mutex ? gps_mutex : (gps_mutex = xSemaphoreCreateMutex()), pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE ? 1 : (ERROR("Failed to acquire mutex"), 0))
#define UNLOCK_MUTEX() do { if (gps_mutex) xSemaphoreGive(gps_mutex); } while(0)
#define RETURN_IF(condition, retval) do { if (condition) return retval; } while(0)

// Global variables
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[SIM4G_GPS_PHONE_MAX_LEN] = DEFAULT_PHONE_NUMBER;
static sim4g_gps_data_t last_known_location = {0};
static TickType_t last_updated = 0;
static atomic_bool is_gps_cold_start = true;
static atomic_uint sms_task_count = ATOMIC_VAR_INIT(0);
static char _temp_buffer[RESPONSE_BUFFER_SIZE];
static uint32_t gps_cold_start_timeout_ms = COLD_START_TIME_MS;
static uint32_t gps_normal_timeout_ms = NORMAL_OPERATION_TIME_MS;
static uint32_t network_check_timeout_ms = NETWORK_CHECK_TIME_MS;

// Struct for SMS task parameters
typedef struct {
    sim4g_gps_data_t location;
    sms_callback_t callback;
} sms_task_params_t;

// Private Function Implementations
static comm_result_t _send_at_command(const char* cmd, char* response, size_t response_size, 
                                     uint8_t retries, uint32_t delay_ms) {
    if (!cmd || !comm_uart_is_initialized()) return COMM_ERROR;

    char* resp = response ? response : _temp_buffer;
    size_t size = response ? response_size : sizeof(_temp_buffer);
    if (resp) resp[0] = '\0';

    comm_result_t res = COMM_ERROR;
    for (uint8_t i = 0; i < retries; i++) {
        if (!LOCK_MUTEX()) continue;
        res = comm_uart_send_command(cmd, resp, size);
        UNLOCK_MUTEX();

        if (res == COMM_SUCCESS && _check_at_response(resp)) return COMM_SUCCESS;
        if (i < retries - 1) DELAY_MS(delay_ms);
    }
    ERROR("AT command %s failed after %d retries", cmd, retries);
    return res;
}

static bool _check_at_response(const char* resp) {
    if (!resp || !resp[0]) return false;
    if (strstr(resp, "ERROR")) {
        int error_code = 0;
        if (sscanf(resp, "+CME ERROR: %d", &error_code) == 1 ||
            sscanf(resp, "+CMS ERROR: %d", &error_code) == 1) {
            ERROR("AT command failed with error code: %d", error_code);
        } else {
            ERROR("AT command failed: %s", resp);
        }
        return false;
    }
    return strstr(resp, "OK") || strstr(resp, "+CMGS:");
}

static bool _retry_command(const char* cmd, char* response, size_t response_size, uint8_t retries, uint32_t delay_ms) {
    if (!cmd) return false;
    comm_result_t res = _send_at_command(cmd, response, response_size, retries, delay_ms);
    return (res == COMM_SUCCESS && _check_at_response(response ? response : _temp_buffer));
}

static float _convert_dmm_to_decimal(const char* dmm, char dir) {
    if (!dmm || strlen(dmm) < 4) return 0.0f;
    int deg_digits = (dir == 'N' || dir == 'S') ? 2 : 3;
    char deg_buf[4] = {0};
    strncpy(deg_buf, dmm, deg_digits);
    float decimal = atoi(deg_buf) + atof(dmm + deg_digits) / 60.0f;
    return (dir == 'S' || dir == 'W') ? -decimal : decimal;
}

static bool _validate_gps_coordinate(const char* coord, char dir) {
    if (!coord || strlen(coord) < GPS_COORD_MIN_LENGTH || strlen(coord) > GPS_COORD_MAX_LENGTH) return false;
    if (dir != 'N' && dir != 'S' && dir != 'E' && dir != 'W') return false;
    for (size_t i = 0; coord[i]; i++) {
        if (coord[i] != '.' && (coord[i] < '0' || coord[i] > '9')) return false;
    }
    float decimal = _convert_dmm_to_decimal(coord, dir);
    return (dir == 'N' || dir == 'S') ? (decimal >= -90.0f && decimal <= 90.0f) :
           (decimal >= -180.0f && decimal <= 180.0f);
}

static bool _check_network_status(void) {
    int mode = 0, status = 0, rssi = 0;
    bool is_registered = false;

    if (_retry_command(AT_CMD_NETWORK_REG, _temp_buffer, sizeof(_temp_buffer), 1, 0) &&
        sscanf(_temp_buffer, "+CREG: %d,%d", &mode, &status) == 2) {
        is_registered = (status == 1 || status == 5);
    }

    if (_retry_command(AT_CMD_SIGNAL_STRENGTH, _temp_buffer, sizeof(_temp_buffer), 1, 0)) {
        sscanf(_temp_buffer, "+CSQ: %d", &rssi);
    }

    return is_registered && (rssi >= MIN_SIGNAL_STRENGTH && rssi != 99);
}

static bool _format_alert_message(char* buffer, size_t size, const sim4g_gps_data_t *location) {
    if (!buffer || !location || !size) return false;
    float lat = _convert_dmm_to_decimal(location->latitude, location->lat_dir[0]);
    float lon = _convert_dmm_to_decimal(location->longitude, location->lon_dir[0]);
    return snprintf(buffer, size, "ALERT: Fall detected!\nLocation: %.5f,%.5f\nTime: %s\nGoogle Maps: https://maps.google.com/?q=%.5f,%.5f",
                   lat, lon, location->timestamp, lat, lon) < (int)size;
}

static bool _send_sms(const char* recipient, const sim4g_gps_data_t* location, char* custom_msg) {
    if (!recipient || !recipient[0] || (!location && !custom_msg)) return false;
    if (strlen(recipient) >= SIM4G_GPS_PHONE_MAX_LEN || (recipient[0] != '+' && (recipient[0] < '0' || recipient[0] > '9'))) return false;

    char sms[SMS_BUFFER_SIZE] = {0};
    if (location && !_format_alert_message(sms, sizeof(sms), location)) return false;
    else if (custom_msg) strncpy(sms, custom_msg, sizeof(sms) - 1);

    // Bước 1: Cấu hình chế độ SMS
    if (!_retry_command(AT_CMD_SMS_TEXT_MODE, NULL, 0, MAX_SMS_RETRY_COUNT, RETRY_DELAY_MS)) return false;
    DELAY_MS(SMS_CONFIG_DELAY_MS);

    // Bước 2: Gửi header SMS
    char cmd[64];
    snprintf(cmd, sizeof(cmd), AT_CMD_SMS_SEND_HEADER, recipient);
    if (!_retry_command(cmd, NULL, 0, MAX_SMS_RETRY_COUNT, RETRY_DELAY_MS)) return false;
    DELAY_MS(SMS_RECIPIENT_DELAY_MS);

    // Bước 3: Gửi nội dung SMS
    if (!_retry_command(sms, NULL, 0, 1, 0)) return false;
    DELAY_MS(SMS_MESSAGE_DELAY_MS);

    // Bước 4: Kết thúc SMS
    if (!_retry_command(AT_CMD_SMS_END, _temp_buffer, sizeof(_temp_buffer), 1, 0)) return false;

    DELAY_MS(SMS_POST_SEND_DELAY_MS);
    return true;
}

static void _sms_sending_task(void *param) {
    sms_task_params_t params = *(sms_task_params_t*)param;
    vPortFree(param);

    TickType_t start_time = xTaskGetTickCount();
    if (!params.location.valid) {
        ERROR("Invalid SMS task parameters");
        if (params.callback) params.callback(false);
        atomic_fetch_sub(&sms_task_count, 1);
        vTaskDelete(NULL);
        return;
    }

    // Kiểm tra timeout tổng quát
    bool network_ready = false;
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(network_check_timeout_ms)) {
        if (_check_network_status()) {
            network_ready = true;
            break;
        }
        if ((xTaskGetTickCount() - start_time) >= pdMS_TO_TICKS(SMS_TOTAL_TIMEOUT_MS)) {
            ERROR("SMS task timed out during network check");
            if (params.callback) params.callback(false);
            atomic_fetch_sub(&sms_task_count, 1);
            vTaskDelete(NULL);
            return;
        }
        DELAY_MS(NETWORK_CHECK_INTERVAL_MS);
    }

    bool sms_sent = false;
    if (network_ready) {
        char recipient[SIM4G_GPS_PHONE_MAX_LEN];
        if (LOCK_MUTEX()) {
            strncpy(recipient, phone_number, sizeof(recipient));
            UNLOCK_MUTEX();
            for (int i = 0; i < MAX_SMS_RETRY_COUNT; i++) {
                if (_send_sms(recipient, ¶ms.location, NULL)) {
                    sms_sent = true;
                    break;
                }
                if (i < MAX_SMS_RETRY_COUNT - 1) {
                    if ((xTaskGetTickCount() - start_time) >= pdMS_TO_TICKS(SMS_TOTAL_TIMEOUT_MS)) {
                        ERROR("SMS task timed out during retry");
                        break;
                    }
                    DELAY_MS(SMS_RETRY_INTERVAL_MS);
                }
            }
        }
    } else {
        ERROR("Network unavailable after %d ms", network_check_timeout_ms);
    }

    if (params.callback) params.callback(network_ready && sms_sent);
    atomic_fetch_sub(&sms_task_count, 1);
    vTaskDelete(NULL);
}

// Public API Implementation
void sim4g_gps_init_with_timeout(uint32_t cold_start_ms, uint32_t normal_ms, uint32_t network_ms) {
    gps_cold_start_timeout_ms = cold_start_ms;
    gps_normal_timeout_ms = normal_ms;
    network_check_timeout_ms = network_ms;

    if (!comm_uart_is_initialized() || !LOCK_MUTEX()) return;

    _retry_command(AT_CMD_GPS_AUTOSTART, NULL, 0, 1, RETRY_DELAY_MS);
    DELAY_MS(SMS_CONFIG_DELAY_MS);

    if (_retry_command(AT_CMD_GPS_ENABLE, _temp_buffer, sizeof(_temp_buffer), 1, RETRY_DELAY_MS)) {
        atomic_store(&is_gps_cold_start, false);
    }
    UNLOCK_MUTEX();
}

void sim4g_gps_init(void) {
    sim4g_gps_init_with_timeout(COLD_START_TIME_MS, NORMAL_OPERATION_TIME_MS, NETWORK_CHECK_TIME_MS);
}

sim4g_error_t sim4g_gps_set_phone_number(const char *phone) {
    if (!phone || strlen(phone) >= SIM4G_GPS_PHONE_MAX_LEN || (phone[0] != '+' && (phone[0] < '0' || phone[0] > '9'))) return SIM4G_ERROR_INVALID_PARAM;
    if (!LOCK_MUTEX()) return SIM4G_ERROR_MUTEX;
    strncpy(phone_number, phone, SIM4G_GPS_PHONE_MAX_LEN - 1);
    UNLOCK_MUTEX();
    return SIM4G_SUCCESS;
}

sim4g_gps_data_t sim4g_gps_get_location(void) {
    sim4g_gps_data_t location = {0};
    uint32_t timeout = atomic_load(&is_gps_cold_start) ? gps_cold_start_timeout_ms : gps_normal_timeout_ms;
    uint32_t start_time = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout)) {
        if (_retry_command(AT_CMD_GPS_LOCATION, _temp_buffer, sizeof(_temp_buffer), 1, 0) &&
            strstr(_temp_buffer, "+QGPSLOC:")) {
            char timestamp[SIM4G_GPS_STRING_MAX_LEN] = {0};
            char latitude[SIM4G_GPS_STRING_MAX_LEN] = {0};
            char lat_dir[2] = {0};
            char longitude[SIM4G_GPS_STRING_MAX_LEN] = {0};
            char lon_dir[2] = {0};

            if (sscanf(_temp_buffer, "+QGPSLOC: %19[^,],%19[^,],%1[^,],%19[^,],%1[^,]",
                       timestamp, latitude, lat_dir, longitude, lon_dir) == 5 &&
                _validate_gps_coordinate(latitude, lat_dir[0]) &&
                _validate_gps_coordinate(longitude, lon_dir[0])) {
                strncpy(location.timestamp, timestamp, sizeof(location.timestamp) - 1);
                strncpy(location.latitude, latitude, sizeof(location.latitude) - 1);
                strncpy(location.lat_dir, lat_dir, sizeof(location.lat_dir) - 1);
                strncpy(location.longitude, longitude, sizeof(location.longitude) - 1);
                strncpy(location.lon_dir, lon_dir, sizeof(location.lon_dir) - 1);
                location.valid = 1;

                if (LOCK_MUTEX()) {
                    memcpy(&last_known_location, &location, sizeof(sim4g_gps_data_t));
                    last_updated = xTaskGetTickCount();
                    atomic_store(&is_gps_cold_start, false);
                    UNLOCK_MUTEX();
                }
                break;
            }
        }
        DELAY_MS(GPS_LOCATION_RETRY_INTERVAL_MS);
    }

    if (!location.valid && LOCK_MUTEX()) {
        if (last_known_location.valid && (xTaskGetTickCount() - last_updated) < pdMS_TO_TICKS(LOCATION_VALIDITY_MS)) {
            location = last_known_location;
        }
        UNLOCK_MUTEX();
    }
    return location;
}

sim4g_error_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location, sms_callback_t callback) {
    if (!location || !location->valid) return SIM4G_ERROR_INVALID_PARAM;
    if (atomic_fetch_add(&sms_task_count, 1) >= MAX_SMS_TASKS) {
        atomic_fetch_sub(&sms_task_count, 1);
        return SIM4G_ERROR_MEMORY;
    }

    sms_task_params_t *params = pvPortMalloc(sizeof(sms_task_params_t));
    if (!params) {
        atomic_fetch_sub(&sms_task_count, 1);
        return SIM4G_ERROR_MEMORY;
    }

    params->location = *location;
    params->callback = callback;
    if (xTaskCreate(_sms_sending_task, "sms_task", SMS_TASK_STACK_SIZE, params, SMS_TASK_PRIORITY, NULL) != pdPASS) {
        vPortFree(params);
        atomic_fetch_sub(&sms_task_count, 1);
        return SIM4G_ERROR_MEMORY;
    }
    return SIM4G_SUCCESS;
}