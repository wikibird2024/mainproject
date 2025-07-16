#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <stdlib.h>
<<<<<<< HEAD
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

=======
#include <math.h>
#include <stdatomic.h>
#include "esp_log.h"
#include <regex.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TAG "SIM4G_GPS"
#define DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define RETURN_IF(condition, retval) do { if (condition) return retval; } while(0)

/*------------------ Module Configuration ------------------*/
static sim4g_gps_config_t gps_config = {
    .default_phone_number = "+84901234567",
    .cold_start_time_ms = 60000,
    .normal_operation_time_ms = 10000,
    .network_check_time_ms = 5000,
    .network_check_interval_ms = 1000,
    .min_signal_strength = 5,
    .max_sms_retry_count = 3,
    .response_buffer_size = 256,
    .retry_delay_ms = 500,
    .sms_buffer_size = 200,
    .mutex_timeout_ms = 1000,
    .gps_location_retry_interval_ms = 1000,
    .sms_retry_interval_ms = 1200,
    .sms_config_delay_ms = 200,
    .sms_recipient_delay_ms = 300,
    .sms_message_delay_ms = 200,
    .sms_post_send_delay_ms = 1200,
    .sms_total_timeout_ms = 15000,
    .location_validity_ms = 300000,
    .max_sms_tasks = 2,
    .gps_coord_min_length = 8,
    .gps_coord_max_length = 12,
    .sms_task_stack_size = 4096,
    .sms_task_priority = 5,
};

/*------------------ Module State ------------------*/
static struct {
    SemaphoreHandle_t mutex;
    char phone_number[SIM4G_GPS_PHONE_MAX_LEN];
    sim4g_gps_data_t last_known_location;
    TickType_t last_updated;
    atomic_bool is_gps_cold_start;
    atomic_uint sms_task_count;
    char temp_buffer[256];
    uint32_t cold_start_timeout_ms;
    uint32_t normal_timeout_ms;
    uint32_t network_check_timeout_ms;
} module_state = {
    .mutex = NULL,
    .phone_number = {0},
    .last_known_location = {0},
    .last_updated = 0,
    .is_gps_cold_start = true,
    .sms_task_count = 0,
    .temp_buffer = {0},
    .cold_start_timeout_ms = 0,
    .normal_timeout_ms = 0,
    .network_check_timeout_ms = 0
};

/*------------------ AT Command Definitions ------------------*/
typedef enum {
    AT_GPS_MODE,
    AT_GPS_ENABLE,
    AT_GPS_LOCATION,
    AT_NETWORK_REG,
    AT_SIGNAL_STRENGTH,
    AT_SMS_TEXT_MODE,
    AT_SMS_SEND_HEADER,
    AT_SMS_END,
    AT_GPS_AUTOSTART,
    AT_CMD_COUNT
} at_cmd_id_t;
static const char* const at_cmd_table[] = {

    [AT_GPS_MODE]        = "AT+choce"
    [AT_GPS_ENABLE]      = "AT+QGPS=1",
    [AT_GPS_LOCATION]    = "AT+QGPSLOC?",
    [AT_NETWORK_REG]     = "AT+CREG?",
    [AT_SIGNAL_STRENGTH] = "AT+CSQ",
    [AT_SMS_TEXT_MODE]   = "AT+CMGF=1",
    [AT_SMS_SEND_HEADER] = "AT+CMGS=\"%s\"",
    [AT_SMS_END]         = "\x1A",
    [AT_GPS_AUTOSTART]   = "AT+QGPSCFG=\"autogps\",1"
};

/*------------------ Mutex Management ------------------*/
static inline bool lock_mutex(void) {
    if (!module_state.mutex) {
        module_state.mutex = xSemaphoreCreateMutex();
        RETURN_IF(!module_state.mutex, false);
    }
    RETURN_IF(xSemaphoreTake(module_state.mutex, pdMS_TO_TICKS(gps_config.mutex_timeout_ms)) != pdTRUE, false);
    return true;
}

static inline void unlock_mutex(void) {
    if (module_state.mutex) {
        xSemaphoreGive(module_state.mutex);
    }
}

/*------------------ Response Parsing ------------------*/
static bool parse_gpsloc(const char* resp, char* timestamp, char* latitude, 
                        char* lat_dir, char* longitude, char* lon_dir) {
    regex_t regex;
    regmatch_t matches[6];
    const char* pattern = "\\+QGPSLOC: *([^,]*),([^,]*),([^,]*),([^,]*),([^,]*)";
    
    RETURN_IF(regcomp(&regex, pattern, REG_EXTENDED) != 0, false);
    
    bool result = false;
    if (regexec(&regex, resp, 6, matches, 0) == 0) {
        #define COPY_MATCH(dest, match) \
            snprintf(dest, matches[match].rm_eo - matches[match].rm_so + 1, \
                    "%.*s", (int)(matches[match].rm_eo - matches[match].rm_so), \
                    resp + matches[match].rm_so)
        
        COPY_MATCH(timestamp, 1);
        COPY_MATCH(latitude, 2);
        COPY_MATCH(lat_dir, 3);
        COPY_MATCH(longitude, 4);
        COPY_MATCH(lon_dir, 5);
        result = true;
        
        #undef COPY_MATCH
    }
    
    regfree(&regex);
    return result;
}

static bool parse_network_status(const char* resp, int* mode, int* status) {
    regex_t regex;
    regmatch_t matches[3];
    const char* pattern = "\\+CREG: *([0-9]+),([0-9]+)";
    
    RETURN_IF(regcomp(&regex, pattern, REG_EXTENDED) != 0, false);
    
    bool result = false;
    if (regexec(&regex, resp, 3, matches, 0) == 0) {
        char mode_buf[8], status_buf[8];
        snprintf(mode_buf, sizeof(mode_buf), "%.*s", 
                (int)(matches[1].rm_eo - matches[1].rm_so), resp + matches[1].rm_so);
        snprintf(status_buf, sizeof(status_buf), "%.*s", 
                (int)(matches[2].rm_eo - matches[2].rm_so), resp + matches[2].rm_so);
        *mode = atoi(mode_buf);
        *status = atoi(status_buf);
        result = true;
    }
    
    regfree(&regex);
    return result;
}

static bool parse_signal_strength(const char* resp, int* rssi) {
    regex_t regex;
    regmatch_t matches[2];
    const char* pattern = "\\+CSQ: *([0-9]+)";
    
    RETURN_IF(regcomp(&regex, pattern, REG_EXTENDED) != 0, false);
    
    bool result = false;
    if (regexec(&regex, resp, 2, matches, 0) == 0) {
        char rssi_buf[8];
        snprintf(rssi_buf, sizeof(rssi_buf), "%.*s", 
                (int)(matches[1].rm_eo - matches[1].rm_so), resp + matches[1].rm_so);
        *rssi = atoi(rssi_buf);
        result = true;
    }
    
    regfree(&regex);
    return result;
}

/*------------------ AT Command Helpers ------------------*/
static comm_result_t send_at_command(const char* cmd, char* response, size_t response_size, 
                                   uint8_t retries, uint32_t delay_ms) {
    RETURN_IF(!cmd || !comm_uart_is_initialized(), COMM_ERROR);
    
    char* resp = response ? response : module_state.temp_buffer;
    size_t size = response ? response_size : sizeof(module_state.temp_buffer);
    if (resp) resp[0] = '\0';
    
    for (uint8_t i = 0; i < retries; i++) {
        if (!lock_mutex()) continue;
        
        comm_result_t res = comm_uart_send_command(cmd, resp, size);
        unlock_mutex();
        
        if (res == COMM_SUCCESS && strstr(resp, "OK")) {
            return COMM_SUCCESS;
        }
        
        if (i < retries - 1) DELAY_MS(delay_ms);
    }
    
    ESP_LOGE(TAG, "AT command '%s' failed after %d retries", cmd, retries);
    return COMM_ERROR;
}

static bool check_at_response(const char* resp) {
    RETURN_IF(!resp || !resp[0], false);
    
    if (strstr(resp, "ERROR")) {
        int error_code = 0;
        if (sscanf(resp, "+CME ERROR: %d", &error_code) == 1 ||
            sscanf(resp, "+CMS ERROR: %d", &error_code) == 1) {
            ESP_LOGE(TAG, "AT command failed with error code: %d", error_code);
        } else {
            ESP_LOGE(TAG, "AT command failed: %s", resp);
        }
        return false;
    }
    
    return strstr(resp, "OK") || strstr(resp, "+CMGS:");
}

static bool retry_command(at_cmd_id_t cmd_id, char* response, size_t response_size, 
                         uint8_t retries, uint32_t delay_ms, ...) {
    RETURN_IF(cmd_id >= AT_CMD_COUNT, false);
    
    char cmd[64];
    va_list args;
    va_start(args, delay_ms);
    vsnprintf(cmd, sizeof(cmd), at_cmd_table[cmd_id], va_arg(args, char*));
    va_end(args);
    
    comm_result_t res = send_at_command(cmd, response, response_size, retries, delay_ms);
    return (res == COMM_SUCCESS && check_at_response(response ? response : module_state.temp_buffer));
}

static bool retry_command_simple(at_cmd_id_t cmd_id, char* response, size_t response_size, 
                                uint8_t retries, uint32_t delay_ms) {
    RETURN_IF(cmd_id >= AT_CMD_COUNT, false);
    comm_result_t res = send_at_command(at_cmd_table[cmd_id], response, response_size, retries, delay_ms);
    return (res == COMM_SUCCESS && check_at_response(response ? response : module_state.temp_buffer));
}

/*------------------ GPS Coordinate Helpers ------------------*/
static float convert_dmm_to_decimal(const char* dmm, char dir) {
    RETURN_IF(!dmm || strlen(dmm) < 4, 0.0f);
    
    int deg_digits = (dir == 'N' || dir == 'S') ? 2 : 3;
    char deg_buf[4] = {0};
    strncpy(deg_buf, dmm, deg_digits);
    
    float decimal = atoi(deg_buf) + atof(dmm + deg_digits) / 60.0f;
    return (dir == 'S' || dir == 'W') ? -decimal : decimal;
}

static bool validate_gps_coordinate(const char* coord, char dir) {
    RETURN_IF(!coord, false);
    
    size_t len = strlen(coord);
    RETURN_IF(len < gps_config.gps_coord_min_length || len > gps_config.gps_coord_max_length, false);
    RETURN_IF(dir != 'N' && dir != 'S' && dir != 'E' && dir != 'W', false);
    
    for (size_t i = 0; i < len; i++) {
        RETURN_IF(coord[i] != '.' && (coord[i] < '0' || coord[i] > '9'), false);
    }
    
    float decimal = convert_dmm_to_decimal(coord, dir);
    return (dir == 'N' || dir == 'S') ? (decimal >= -90.0f && decimal <= 90.0f) :
           (decimal >= -180.0f && decimal <= 180.0f);
}

/*------------------ Network Helpers ------------------*/
static bool check_network_status(void) {
    int mode = 0, status = 0, rssi = 0;
    bool is_registered = false;
    
    if (retry_command_simple(AT_NETWORK_REG, module_state.temp_buffer, 
                           sizeof(module_state.temp_buffer), 1, 0) &&
        parse_network_status(module_state.temp_buffer, &mode, &status)) {
        is_registered = (status == 1 || status == 5);
    }
    
    if (retry_command_simple(AT_SIGNAL_STRENGTH, module_state.temp_buffer,
                           sizeof(module_state.temp_buffer), 1, 0)) {
        parse_signal_strength(module_state.temp_buffer, &rssi);
    }
    
    return is_registered && (rssi >= gps_config.min_signal_strength && rssi != 99);
}

/*------------------ SMS Helpers ------------------*/
static bool format_alert_message(char* buffer, size_t size, const sim4g_gps_data_t *location) {
    RETURN_IF(!buffer || !location || !size, false);
    
    float lat = convert_dmm_to_decimal(location->latitude, location->lat_dir[0]);
    float lon = convert_dmm_to_decimal(location->longitude, location->lon_dir[0]);
    
    int written = snprintf(buffer, size, 
                         "ALERT: Fall detected!\n"
                         "Location: %.5f,%.5f\n"
                         "Time: %s\n"
                         "Google Maps: https://maps.google.com/?q=%.5f,%.5f",
                         lat, lon, location->timestamp, lat, lon);
    
    buffer[size - 1] = '\0';
    return written > 0 && (size_t)written < size;
}

static bool send_sms(const char* recipient, const sim4g_gps_data_t* location, const char* custom_msg) {
    RETURN_IF(!recipient || !recipient[0] || (!location && !custom_msg), false);
    RETURN_IF(strlen(recipient) >= SIM4G_GPS_PHONE_MAX_LEN || 
             (recipient[0] != '+' && (recipient[0] < '0' || recipient[0] > '9')), false);
    
    char sms[gps_config.sms_buffer_size];
    memset(sms, 0, sizeof(sms));
    
    if (location && !format_alert_message(sms, sizeof(sms), location)) {
        return false;
    } else if (custom_msg) {
        strncpy(sms, custom_msg, sizeof(sms) - 1);
    }
    
    if (!retry_command_simple(AT_SMS_TEXT_MODE, NULL, 0, gps_config.max_sms_retry_count, gps_config.retry_delay_ms)) {
        return false;
    }
    DELAY_MS(gps_config.sms_config_delay_ms);
    
    if (!retry_command(AT_SMS_SEND_HEADER, NULL, 0, gps_config.max_sms_retry_count, 
                      gps_config.retry_delay_ms, recipient)) {
        return false;
    }
    DELAY_MS(gps_config.sms_recipient_delay_ms);
    
    if (!send_at_command(sms, NULL, 0, 1, 0)) {
        return false;
    }
    DELAY_MS(gps_config.sms_message_delay_ms);
    
    if (!retry_command_simple(AT_SMS_END, module_state.temp_buffer, 
                            sizeof(module_state.temp_buffer), 1, 0)) {
        return false;
    }
    DELAY_MS(gps_config.sms_post_send_delay_ms);
    
    return true;
}

/*------------------ SMS Task Implementation ------------------*/
typedef struct {
    sim4g_gps_data_t location;
    sms_callback_t callback;
} sms_task_params_t;

static void sms_sending_task(void *param) {
    sms_task_params_t params = *(sms_task_params_t*)param;
    vPortFree(param);
    
    TickType_t start_time = xTaskGetTickCount();
    RETURN_IF(!params.location.valid, );
    
    // Wait for network to become available
    bool network_ready = false;
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(module_state.network_check_timeout_ms)) {
        if (check_network_status()) {
            network_ready = true;
            break;
        }
        DELAY_MS(gps_config.network_check_interval_ms);
    }
    
    // Send SMS if network is ready
    bool sms_sent = false;
    if (network_ready) {
        char recipient[SIM4G_GPS_PHONE_MAX_LEN];
        if (lock_mutex()) {
            strncpy(recipient, module_state.phone_number, sizeof(recipient) - 1);
            recipient[sizeof(recipient) - 1] = '\0';
            unlock_mutex();
            
            for (int i = 0; i < gps_config.max_sms_retry_count; i++) {
                if (send_sms(recipient, &params.location, NULL)) {
                    sms_sent = true;
                    break;
                }
                DELAY_MS(gps_config.sms_retry_interval_ms);
            }
        }
    } else {
        ESP_LOGE(TAG, "Network unavailable after %d ms", module_state.network_check_timeout_ms);
    }
    
    // Notify callback and clean up
    if (params.callback) {
        params.callback(network_ready && sms_sent);
    }
    
    atomic_fetch_sub(&module_state.sms_task_count, 1);
    vTaskDelete(NULL);
}

/*------------------ Public API Implementation ------------------*/
void sim4g_set_config(const sim4g_gps_config_t* new_cfg) {
    if (new_cfg && lock_mutex()) {
        memcpy(&gps_config, new_cfg, sizeof(sim4g_gps_config_t));
        unlock_mutex();
    }
}

void sim4g_gps_init_with_timeout(uint32_t cold_start_ms, uint32_t normal_ms, uint32_t network_ms) {
    module_state.cold_start_timeout_ms = cold_start_ms;
    module_state.normal_timeout_ms = normal_ms;
    module_state.network_check_timeout_ms = network_ms;
    
    if (!comm_uart_is_initialized() || !lock_mutex()) {
        ESP_LOGE(TAG, "UART not initialized or failed to acquire mutex during init");
        return;
    }
    
    retry_command_simple(AT_GPS_AUTOSTART, NULL, 0, 1, gps_config.retry_delay_ms);
    DELAY_MS(gps_config.sms_config_delay_ms);
    
    if (retry_command_simple(AT_GPS_ENABLE, module_state.temp_buffer, 
                           sizeof(module_state.temp_buffer), 1, gps_config.retry_delay_ms)) {
        atomic_store(&module_state.is_gps_cold_start, false);
    }
    
    unlock_mutex();
    strncpy(module_state.phone_number, gps_config.default_phone_number, 
           SIM4G_GPS_PHONE_MAX_LEN - 1);
    module_state.phone_number[SIM4G_GPS_PHONE_MAX_LEN - 1] = '\0';
}

void sim4g_gps_init(void) {
    sim4g_gps_init_with_timeout(
        gps_config.cold_start_time_ms,
        gps_config.normal_operation_time_ms,
        gps_config.network_check_time_ms
    );
}

sim4g_error_t sim4g_gps_set_phone_number(const char *phone) {
    RETURN_IF(!phone || strlen(phone) >= SIM4G_GPS_PHONE_MAX_LEN || 
             (phone[0] != '+' && (phone[0] < '0' || phone[0] > '9')), 
             SIM4G_ERROR_INVALID_PARAM);
    
    if (!lock_mutex()) return SIM4G_ERROR_MUTEX;
    
    strncpy(module_state.phone_number, phone, SIM4G_GPS_PHONE_MAX_LEN - 1);
    module_state.phone_number[SIM4G_GPS_PHONE_MAX_LEN - 1] = '\0';
    
    unlock_mutex();
    return SIM4G_SUCCESS;
}

sim4g_gps_data_t sim4g_gps_get_location(void) {
    sim4g_gps_data_t location = {0};
    uint32_t timeout = atomic_load(&module_state.is_gps_cold_start) ? 
                      module_state.cold_start_timeout_ms : module_state.normal_timeout_ms;
    TickType_t start_time = xTaskGetTickCount();
    
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout)) {
        if (retry_command_simple(AT_GPS_LOCATION, module_state.temp_buffer, 
                               sizeof(module_state.temp_buffer), 1, 0) &&
            strstr(module_state.temp_buffer, "+QGPSLOC:")) {
            
            char timestamp[SIM4G_GPS_STRING_MAX_LEN] = {0};
            char latitude[SIM4G_GPS_STRING_MAX_LEN] = {0};
            char lat_dir[2] = {0};
            char longitude[SIM4G_GPS_STRING_MAX_LEN] = {0};
            char lon_dir[2] = {0};
            
            if (parse_gpsloc(module_state.temp_buffer, timestamp, latitude, 
                            lat_dir, longitude, lon_dir) &&
                validate_gps_coordinate(latitude, lat_dir[0]) &&
                validate_gps_coordinate(longitude, lon_dir[0])) {
                
                // Copy valid location data
                #define COPY_FIELD(field) \
                    strncpy(location.field, field, sizeof(location.field) - 1); \
                    location.field[sizeof(location.field) - 1] = '\0'
                
                COPY_FIELD(timestamp);
                COPY_FIELD(latitude);
                COPY_FIELD(lat_dir);
                COPY_FIELD(longitude);
                COPY_FIELD(lon_dir);
                location.valid = 1;
                
                #undef COPY_FIELD
                
                // Update last known location
                if (lock_mutex()) {
                    memcpy(&module_state.last_known_location, &location, sizeof(sim4g_gps_data_t));
                    module_state.last_updated = xTaskGetTickCount();
                    atomic_store(&module_state.is_gps_cold_start, false);
                    unlock_mutex();
                }
                break;
            }
        }
        DELAY_MS(gps_config.gps_location_retry_interval_ms);
    }
    
    // Fall back to last known location if current attempt failed
    if (!location.valid && lock_mutex()) {
        if (module_state.last_known_location.valid && 
            (xTaskGetTickCount() - module_state.last_updated) < pdMS_TO_TICKS(gps_config.location_validity_ms)) {
            location = module_state.last_known_location;
        }
        unlock_mutex();
    }
    
    return location;
}

sim4g_error_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location, sms_callback_t callback) {
    RETURN_IF(!location || !location->valid, SIM4G_ERROR_INVALID_PARAM);
    
    // Check if we can spawn another SMS task
    uint32_t current_tasks = atomic_fetch_add(&module_state.sms_task_count, 1);
    RETURN_IF(current_tasks >= gps_config.max_sms_tasks, SIM4G_ERROR_MEMORY);
    
    // Allocate and prepare task parameters
    sms_task_params_t *params = pvPortMalloc(sizeof(sms_task_params_t));
    RETURN_IF(!params, SIM4G_ERROR_MEMORY);
    
    params->location = *location;
    params->callback = callback;
    
    // Create the SMS sending task
    if (xTaskCreate(sms_sending_task, "sms_task", gps_config.sms_task_stack_size, 
                   params, gps_config.sms_task_priority, NULL) != pdPASS) {
        vPortFree(params);
        atomic_fetch_sub(&module_state.sms_task_count, 1);
        return SIM4G_ERROR_MEMORY;
    }
    
    return SIM4G_SUCCESS;
}
>>>>>>> 2c4b6e7 (add to rebase)
