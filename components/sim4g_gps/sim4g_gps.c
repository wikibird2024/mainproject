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
#define DEFAULT_PHONE_NUMBER        "+84901234567"
#define COLD_START_TIME_MS          60000
#define NORMAL_OPERATION_TIME_MS    10000
#define NETWORK_CHECK_TIME_MS       5000
#define NETWORK_CHECK_INTERVAL_MS   1000
#define MIN_SIGNAL_STRENGTH         5
#define MAX_SMS_RETRY_COUNT         3
#define RESPONSE_BUFFER_SIZE        256
#define RETRY_DELAY_MS              500
#define SMS_BUFFER_SIZE             200
#define MUTEX_TIMEOUT_MS            1000
#define GPS_LOCATION_RETRY_INTERVAL_MS 1000
#define SMS_RETRY_INTERVAL_MS       1200
#define SMS_CONFIG_DELAY_MS         200
#define SMS_RECIPIENT_DELAY_MS      300
#define LOCATION_VALIDITY_MS        300000 // 5 phút
#define MAX_SMS_TASKS               2

// GPS validation constants
#define GPS_COORD_MIN_LENGTH        8
#define GPS_COORD_MAX_LENGTH        12
#define GPS_COORDINATE_PRECISION    5

// Task configuration
#define SMS_TASK_STACK_SIZE         4096
#define SMS_TASK_PRIORITY           5

// AT Commands
#define AT_CMD_GPS_ENABLE           "AT+QGPS=1"
#define AT_CMD_GPS_LOCATION         "AT+QGPSLOC?"
#define AT_CMD_NETWORK_REG          "AT+CREG?"
#define AT_CMD_SIGNAL_STRENGTH      "AT+CSQ"
#define AT_CMD_SMS_TEXT_MODE        "AT+CMGF=1"
#define AT_CMD_SMS_SEND_HEADER      "AT+CMGS=\"%s\""
#define AT_CMD_SMS_END              "\x1A"
#define AT_CMD_GPS_AUTOSTART        "AT+QGPSCFG=\"autogps\",1"

// Helper macros
#define SEND_AT_CMD(cmd, retries) _send_at_command(cmd, NULL, 0, retries, RETRY_DELAY_MS)
#define DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define LOCK_MUTEX() (_take_mutex() ? 1 : (ERROR("Failed to acquire mutex"), 0))
#define UNLOCK_MUTEX() do { if(gps_mutex) xSemaphoreGive(gps_mutex); } while(0)
#define RETURN_IF(condition, retval) do { if (condition) { return retval; } } while(0)
#define LOG_IF_ERROR(condition, message) do { if (!(condition)) { ERROR(message); } } while(0)
#define SAFE_FREE(ptr) do { if (ptr) { vPortFree(ptr); ptr = NULL; } } while(0)

// Global variables
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[SIM4G_GPS_PHONE_MAX_LEN] = DEFAULT_PHONE_NUMBER;
static sim4g_gps_data_t last_known_location = {0};
static TickType_t last_updated = 0; // Thời gian cập nhật last_known_location
static atomic_bool is_gps_cold_start = true;
static atomic_uint sms_task_count = ATOMIC_VAR_INIT(0);
static char _temp_buffer[RESPONSE_BUFFER_SIZE];
static uint32_t gps_cold_start_timeout_ms = COLD_START_TIME_MS;
static uint32_t gps_normal_timeout_ms = NORMAL_OPERATION_TIME_MS;
static uint32_t network_check_timeout_ms = NETWORK_CHECK_TIME_MS;

// Struct for SMS task parameters
typedef struct {
    sim4g_gps_data_t *location;
    sms_callback_t callback;
} sms_task_params_t;

// =============================================================================
// Private Function Implementations
// =============================================================================

static inline bool _ensure_comm_ready() {
    bool ready = comm_uart_is_initialized();
    LOG_IF_ERROR(ready, "UART communication not initialized");
    return ready;
}

static inline bool _ensure_mutex() {
    if (gps_mutex == NULL) {
        gps_mutex = xSemaphoreCreateMutex();
        if (gps_mutex != NULL) {
            DEBUG("GPS mutex created successfully");
            return true;
        }
        ERROR("Failed to create mutex");
        return false;
    }
    return true;
}

static inline bool _take_mutex() {
    if (!_ensure_mutex()) {
        return false;
    }
    return (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) == pdTRUE);
}

static comm_result_t _send_at_command(const char* cmd, char* response, size_t response_size, 
                                    uint8_t retries, uint32_t delay_ms) {
    RETURN_IF(!cmd, COMM_INVALID_PARAM);
    RETURN_IF(!_ensure_comm_ready(), COMM_ERROR);
    
    DEBUG("Sending AT command: %s", cmd);
    
    comm_result_t res = COMM_ERROR;
    char* resp_buffer = response ? response : _temp_buffer;
    size_t resp_size = response ? response_size : sizeof(_temp_buffer);
    
    if (resp_buffer && resp_size > 0) {
        resp_buffer[0] = '\0';
    }
    
    for (uint8_t i = 0; i < retries; i++) {
        if (!_take_mutex()) {
            WARN("Failed to acquire mutex for AT command on attempt %d", i + 1);
            continue;
        }
        
        res = comm_uart_send_command(cmd, resp_buffer, resp_size);
        UNLOCK_MUTEX();
        
        if (res == COMM_SUCCESS) {
            if (resp_buffer[0] && !_check_at_response(resp_buffer)) {
                WARN("AT command returned success but response indicates error: %s", resp_buffer);
                res = COMM_ERROR;
            } else {
                DEBUG("AT command successful on attempt %d", i + 1);
                break;
            }
        } else {
            const char* error_str = (res == COMM_TIMEOUT) ? "timeout" : "error";
            WARN("AT command %s on attempt %d", error_str, i + 1);
        }
        
        if (i < retries - 1) {
            DELAY_MS(delay_ms);
        }
    }
    
    LOG_IF_ERROR(res == COMM_SUCCESS, "AT command failed after maximum retries");
    return res;
}

static bool _validate_gps_coordinate(const char* coord, char direction) {
    RETURN_IF(!coord || !coord[0], false);
    
    size_t len = strlen(coord);
    if (len < GPS_COORD_MIN_LENGTH || len > GPS_COORD_MAX_LENGTH) {
        return false;
    }
    
    if (direction != 'N' && direction != 'S' && direction != 'E' && direction != 'W') {
        return false;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (!(coord[i] == '.' || (coord[i] >= '0' && coord[i] <= '9'))) {
            return false;
        }
    }
    
    float decimal = _convert_dmm_to_decimal(coord, direction);
    if (direction == 'N' || direction == 'S') {
        return (decimal >= -90.0f && decimal <= 90.0f);
    }
    return (decimal >= -180.0f && decimal <= 180.0f);
}

static float _convert_dmm_to_decimal(const char* dmm, char dir) {
    RETURN_IF(!dmm || strlen(dmm) < 4, 0.0f);

    int deg_digits = (dir == 'N' || dir == 'S') ? 2 : 3;
    char deg_buf[4] = {0};
    strncpy(deg_buf, dmm, deg_digits);
    
    int degrees = atoi(deg_buf);
    float minutes = atof(dmm + deg_digits);
    float decimal = degrees + (minutes / 60.0f);
    
    float result = (dir == 'S' || dir == 'W') ? -decimal : decimal;
    DEBUG("Converted %s%c to %.6f", dmm, dir, result);
    
    return result;
}

static bool _check_at_response(const char* resp) {
    RETURN_IF(!resp || resp[0] == '\0', false);
    
    bool has_ok = strstr(resp, "OK") != NULL;
    bool has_cmgs = strstr(resp, "+CMGS:") != NULL;
    bool has_error = strstr(resp, "ERROR") != NULL;
    
    if (has_error) {
        int error_code = 0;
        if (sscanf(resp, "+CME ERROR: %d", &error_code) == 1 ||
            sscanf(resp, "+CMS ERROR: %d", &error_code) == 1) {
            ERROR("AT command failed with error code: %d", error_code);
        } else {
            ERROR("AT command failed with unknown error: %s", resp);
        }
        return false;
    }
    
    return has_ok || has_cmgs;
}

static bool _check_network_status(void) {
    DEBUG("Checking network registration status");
    
    int mode = 0, status = 0;
    bool is_registered = false;
    
    if (_send_at_command(AT_CMD_NETWORK_REG, _temp_buffer, sizeof(_temp_buffer), 1, 0) == COMM_SUCCESS) {
        if (sscanf(_temp_buffer, "+CREG: %d,%d", &mode, &status) == 2) {
            is_registered = (status == 1 || status == 5);
            DEBUG("Network registration: mode=%d, status=%d, registered=%s", 
                  mode, status, is_registered ? "yes" : "no");
        }
    }

    int rssi = 0;
    if (_send_at_command(AT_CMD_SIGNAL_STRENGTH, _temp_buffer, sizeof(_temp_buffer), 1, 0) == COMM_SUCCESS) {
        if (sscanf(_temp_buffer, "+CSQ: %d", &rssi) == 1) {
            DEBUG("Signal strength: %d", rssi);
        } else {
            rssi = 0;
            WARN("Failed to parse signal strength");
        }
    }

    bool network_ready = (is_registered && (rssi >= MIN_SIGNAL_STRENGTH && rssi != 99));
    INFO("Network status: registered=%s, signal=%d, ready=%s", 
         is_registered ? "yes" : "no", rssi, network_ready ? "yes" : "no");
    
    return network_ready;
}

static bool _format_alert_message(char* buffer, size_t size, const sim4g_gps_data_t *location) {
    RETURN_IF(!buffer || !location || size == 0, false);
    
    float lat = _convert_dmm_to_decimal(location->latitude, location->lat_dir[0]);
    float lon = _convert_dmm_to_decimal(location->longitude, location->lon_dir[0]);
    
    int needed = snprintf(buffer, size,
        "ALERT: Fall detected!\nLocation: %.5f,%.5f\nTime: %s\nGoogle Maps: https://maps.google.com/?q=%.5f,%.5f",
        lat, lon, location->timestamp, lat, lon);
    
    RETURN_IF(needed < 0 || needed >= (int)size, false);
    return true;
}

static bool _validate_phone_number(const char *phone_number) {
    RETURN_IF(!phone_number, false);
    size_t len = strlen(phone_number);
    RETURN_IF(len == 0 || len > SIM4G_GPS_PHONE_MAX_LEN - 1, false);

    if (phone_number[0] != '+' && (phone_number[0] < '0' || phone_number[0] > '9')) {
        return false;
    }
    for (size_t i = 1; i < len; i++) {
        if (phone_number[i] < '0' || phone_number[i] > '9') {
            return false;
        }
    }
    return true;
}

static bool _get_current_phone_number(char* buffer, size_t size) {
    RETURN_IF(!buffer || size == 0, false);
    
    if (!LOCK_MUTEX()) {
        return false;
    }
    
    strncpy(buffer, phone_number, size - 1);
    buffer[size - 1] = '\0';
    UNLOCK_MUTEX();
    return true;
}

static bool _parse_gps_response(const char* response, sim4g_gps_data_t* location) {
    RETURN_IF(!response || !location, false);
    
    char timestamp[SIM4G_GPS_STRING_MAX_LEN] = {0};
    char latitude[SIM4G_GPS_STRING_MAX_LEN] = {0};
    char lat_dir[2] = {0};
    char longitude[SIM4G_GPS_STRING_MAX_LEN] = {0};
    char lon_dir[2] = {0};
    
    int parsed = sscanf(response, "+QGPSLOC: %19[^,],%19[^,],%1[^,],%19[^,],%1[^,]",
        timestamp, latitude, lat_dir, longitude, lon_dir);
        
    RETURN_IF(parsed != 5, false);
    
    strncpy(location->timestamp, timestamp, sizeof(location->timestamp) - 1);
    strncpy(location->latitude, latitude, sizeof(location->latitude) - 1);
    strncpy(location->lat_dir, lat_dir, sizeof(location->lat_dir) - 1);
    strncpy(location->longitude, longitude, sizeof(location->longitude) - 1);
    strncpy(location->lon_dir, lon_dir, sizeof(location->lon_dir) - 1);
    
    location->timestamp[sizeof(location->timestamp) - 1] = '\0';
    location->latitude[sizeof(location->latitude) - 1] = '\0';
    location->lat_dir[sizeof(location->lat_dir) - 1] = '\0';
    location->longitude[sizeof(location->longitude) - 1] = '\0';
    location->lon_dir[sizeof(location->lon_dir) - 1] = '\0';
    
    return _validate_gps_coordinate(location->latitude, location->lat_dir[0]) &&
           _validate_gps_coordinate(location->longitude, location->lon_dir[0]);
}

static bool _send_sms(const char* recipient, const char* message) {
    RETURN_IF(!recipient || !recipient[0] || !message || !message[0], false);
    
    INFO("Sending SMS to %s", recipient);
    DEBUG("SMS content: %s", message);

    RETURN_IF(SEND_AT_CMD(AT_CMD_SMS_TEXT_MODE, MAX_SMS_RETRY_COUNT) != COMM_SUCCESS, false);
    DELAY_MS(SMS_CONFIG_DELAY_MS);

    char cmd[64];
    int cmd_len = snprintf(cmd, sizeof(cmd), AT_CMD_SMS_SEND_HEADER, recipient);
    RETURN_IF(cmd_len < 0 || cmd_len >= (int)sizeof(cmd), false);
    
    RETURN_IF(SEND_AT_CMD(cmd, MAX_SMS_RETRY_COUNT) != COMM_SUCCESS, false);
    DELAY_MS(SMS_RECIPIENT_DELAY_MS);

    RETURN_IF(_send_at_command(message, NULL, 0, 1, 0) != COMM_SUCCESS, false);
    DELAY_MS(200);

    RETURN_IF(_send_at_command(AT_CMD_SMS_END, _temp_buffer, sizeof(_temp_buffer), 1, 0) != COMM_SUCCESS, false);
    RETURN_IF(!_check_at_response(_temp_buffer), false);

    INFO("SMS sent successfully to %s", recipient);
    DELAY_MS(1200);
    return true;
}

static bool _send_alert_sms(const sim4g_gps_data_t *location) {
    RETURN_IF(!location || !location->valid, false);
    
    RETURN_IF(!location->latitude[0] || !location->longitude[0] || 
             !location->timestamp[0] || !location->lat_dir[0] || 
             !location->lon_dir[0], false);
    
    RETURN_IF(!_validate_gps_coordinate(location->latitude, location->lat_dir[0]) ||
             !_validate_gps_coordinate(location->longitude, location->lon_dir[0]), false);

    char current_phone[SIM4G_GPS_PHONE_MAX_LEN] = {0};
    RETURN_IF(!_get_current_phone_number(current_phone, sizeof(current_phone)), false);

    char sms[SMS_BUFFER_SIZE] = {0};
    RETURN_IF(!_format_alert_message(sms, sizeof(sms), location), false);

    return _send_sms(current_phone, sms);
}

static void _cleanup_sms_task_params(sms_task_params_t *params) {
    if (params) {
        SAFE_FREE(params->location);
        SAFE_FREE(params);
        DEBUG("SMS task parameters cleaned up");
    }
}

static void _sms_sending_task(void *param) {
    DEBUG("SMS sending task started");
    
    sms_task_params_t *task_params = (sms_task_params_t *)param;
    if (!task_params || !task_params->location) {
        ERROR("Invalid SMS task parameters");
        if (task_params && task_params->callback) {
            task_params->callback(false);
        }
        _cleanup_sms_task_params(task_params);
        atomic_fetch_sub(&sms_task_count, 1);
        vTaskDelete(NULL);
        return;
    }

    uint32_t start_time = xTaskGetTickCount();
    bool network_ready = false;
    bool sms_sent = false;

    INFO("Waiting for network connection...");
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(network_check_timeout_ms)) {
        if (_check_network_status()) {
            network_ready = true;
            break;
        }
        DELAY_MS(NETWORK_CHECK_INTERVAL_MS);
    }

    if (network_ready) {
        INFO("Network ready, attempting to send SMS");
        for (int i = 0; i < MAX_SMS_RETRY_COUNT; i++) {
            if (_send_alert_sms(task_params->location)) {
                INFO("SMS sent successfully on attempt %d", i + 1);
                sms_sent = true;
                break;
            }
            if (i < MAX_SMS_RETRY_COUNT - 1) {
                WARN("SMS send failed, retrying in %d ms", SMS_RETRY_INTERVAL_MS);
                DELAY_MS(SMS_RETRY_INTERVAL_MS);
            }
        }
    } else {
        ERROR("Network unavailable for SMS sending after %d ms timeout", network_check_timeout_ms);
    }

    if (task_params->callback) {
        task_params->callback(network_ready && sms_sent);
    }

    _cleanup_sms_task_params(task_params);
    atomic_fetch_sub(&sms_task_count, 1);
    DEBUG("SMS sending task completed");
    vTaskDelete(NULL);
}

// =============================================================================
// Public API Implementation
// =============================================================================

void sim4g_gps_init_with_timeout(uint32_t cold_start_ms, uint32_t normal_ms, uint32_t network_ms) {
    INFO("Initializing SIM4G GPS module with timeouts: cold=%lu ms, normal=%lu ms, network=%lu ms",
         cold_start_ms, normal_ms, network_ms);
    
    gps_cold_start_timeout_ms = cold_start_ms;
    gps_normal_timeout_ms = normal_ms;
    network_check_timeout_ms = network_ms;

    RETURN_IF(!_ensure_comm_ready(), );
    RETURN_IF(!_ensure_mutex(), );
    
    DEBUG("Configuring GPS autostart");
    SEND_AT_CMD(AT_CMD_GPS_AUTOSTART, 1);
    DELAY_MS(SMS_CONFIG_DELAY_MS);  

    bool is_cold_start = atomic_load(&is_gps_cold_start);
    INFO("GPS starting in %s mode", is_cold_start ? "COLD START" : "HOT START");  

    comm_result_t res = SEND_AT_CMD(AT_CMD_GPS_ENABLE, 1);
    
    if (res == COMM_SUCCESS && _check_at_response(_temp_buffer)) {  
        atomic_store(&is_gps_cold_start, false);  
        INFO("GPS enabled successfully");  
    } else {  
        ERROR("GPS initialization failed with result: %d", res);  
    }
}

void sim4g_gps_init(void) {
    sim4g_gps_init_with_timeout(COLD_START_TIME_MS, NORMAL_OPERATION_TIME_MS, NETWORK_CHECK_TIME_MS);
}

sim4g_error_t sim4g_gps_set_phone_number(const char *phone_number) {
    RETURN_IF(!phone_number, SIM4G_ERROR_INVALID_PARAM);
    RETURN_IF(!_validate_phone_number(phone_number), SIM4G_ERROR_INVALID_PARAM);
    
    if (!LOCK_MUTEX()) {
        return SIM4G_ERROR_MUTEX;
    }
    
    strncpy(phone_number, phone_number, SIM4G_GPS_PHONE_MAX_LEN - 1);
    phone_number[SIM4G_GPS_PHONE_MAX_LEN - 1] = '\0';
    UNLOCK_MUTEX();
    
    INFO("Updated SMS recipient number to: %s", phone_number);
    return SIM4G_SUCCESS;
}

sim4g_gps_data_t sim4g_gps_get_location(void) {
    sim4g_gps_data_t location = {0};
    location.valid = 0;

    bool is_cold_start = atomic_load(&is_gps_cold_start);
    uint32_t gps_timeout = is_cold_start ? gps_cold_start_timeout_ms : gps_normal_timeout_ms;
    uint32_t start_time = xTaskGetTickCount();

    INFO("Getting GPS location (timeout: %lu ms, cold_start: %s)", 
         gps_timeout, is_cold_start ? "yes" : "no");

    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(gps_timeout)) {
        if (_send_at_command(AT_CMD_GPS_LOCATION, _temp_buffer, sizeof(_temp_buffer), 1, 0) == COMM_SUCCESS && 
            strstr(_temp_buffer, "+QGPSLOC:")) {
            
            DEBUG("GPS response: %s", _temp_buffer);
            
            if (_parse_gps_response(_temp_buffer, &location)) {
                location.valid = 1;
                
                if (LOCK_MUTEX()) {
                    memcpy(&last_known_location, &location, sizeof(sim4g_gps_data_t));
                    last_updated = xTaskGetTickCount();
                    atomic_store(&is_gps_cold_start, false);
                    UNLOCK_MUTEX();
                    
                    INFO("GPS location acquired: %.5f,%.5f at %s",
                         _convert_dmm_to_decimal(location.latitude, location->lat_dir[0]),
                         _convert_dmm_to_decimal(location->longitude, location->lon_dir[0]),
                         location.timestamp);
                }
                break;
            }
        }
        
        DELAY_MS(GPS_LOCATION_RETRY_INTERVAL_MS);
    }

    if (!location.valid) {
        WARN("GPS timeout after %lu ms, checking last known location", gps_timeout);
        if (LOCK_MUTEX()) {
            if (last_known_location.valid && 
                (xTaskGetTickCount() - last_updated) < pdMS_TO_TICKS(LOCATION_VALIDITY_MS)) {
                location = last_known_location;
                INFO("Using last known GPS location");
            } else {
                WARN("No valid GPS location available or last known location too old");
            }
            UNLOCK_MUTEX();
        }
    }

    return location;
}

sim4g_error_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location, sms_callback_t callback) {
    RETURN_IF(!location || !location->valid, SIM4G_ERROR_INVALID_PARAM);
    
    if (atomic_fetch_add(&sms_task_count, 1) >= MAX_SMS_TASKS) {
        atomic_fetch_sub(&sms_task_count, 1);
        ERROR("Maximum SMS tasks reached");
        return SIM4G_ERROR_MEMORY;
    }
    
    if (esp_get_free_heap_size() < 2048) {
        atomic_fetch_sub(&sms_task_count, 1);
        ERROR("Insufficient heap memory");
        return SIM4G_ERROR_MEMORY;
    }

    INFO("Initiating async fall alert SMS");

    sms_task_params_t *params = pvPortMalloc(sizeof(sms_task_params_t));  
    RETURN_IF(!params, SIM4G_ERROR_MEMORY);  

    params->location = pvPortMalloc(sizeof(sim4g_gps_data_t));  
    if (!params->location) {  
        vPortFree(params);
        atomic_fetch_sub(&sms_task_count, 1);
        return SIM4G_ERROR_MEMORY;  
    }  

    memcpy(params->location, location, sizeof(sim4g_gps_data_t));  
    params->callback = callback;

    if (xTaskCreate(_sms_sending_task, "sms_task", SMS_TASK_STACK_SIZE, params, SMS_TASK_PRIORITY, NULL) != pdPASS) {  
        _cleanup_sms_task_params(params);
        atomic_fetch_sub(&sms_task_count, 1);
        return SIM4G_ERROR_MEMORY;
    }
    
    DEBUG("SMS sending task created successfully");
    return SIM4G_SUCCESS;
}