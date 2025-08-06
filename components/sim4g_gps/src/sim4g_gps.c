#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Cần cho atof()

#include "esp_log.h"
#include "esp_err.h"

#include "data_manager.h" // Thêm data_manager
#include "sim4g_at.h"
#include "sim4g_at_cmd.h"
#include "sim4g_gps.h"
#include "sdkconfig.h"

#define DEFAULT_PHONE CONFIG_SIM4G_DEFAULT_PHONE

static const char *TAG = "SIM4G_GPS";

// Internal state
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[16] = DEFAULT_PHONE;

// Internal task for sending SMS
static void send_sms_task(void *param) {
    // Lấy dữ liệu từ tham số và giải phóng bộ nhớ
    sim4g_gps_data_t *loc = (sim4g_gps_data_t *)param;

    if (!loc || !loc->valid) {
        ESP_LOGE(TAG, "Invalid GPS data for SMS task");
        if (loc) {
            free(loc);
        }
        vTaskDelete(NULL);
        return;
    }

    // Tạo nội dung tin nhắn từ dữ liệu GPS kiểu double
    char msg[256];
    snprintf(msg, sizeof(msg), "Fall detected!\nLat: %.6f\nLon: %.6f\nTime: %s",
             loc->latitude, loc->longitude, loc->timestamp);

    ESP_LOGI(TAG, "Sending SMS to %s:\n%s", phone_number, msg);

    esp_err_t err = sim4g_at_send_sms(phone_number, msg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SMS send failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "SMS sent successfully");
    }

    free(loc); // Giải phóng bộ nhớ đã cấp phát
    vTaskDelete(NULL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t sim4g_gps_init(void) {
    if (!gps_mutex) {
        gps_mutex = xSemaphoreCreateMutex();
        if (!gps_mutex) {
            ESP_LOGE(TAG, "Failed to create GPS mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    esp_err_t err = sim4g_at_enable_gps();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "GPS enabled successfully");
    } else {
        ESP_LOGE(TAG, "GPS initialization failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t sim4g_gps_set_phone_number(const char *number) {
    if (!number || strlen(number) >= sizeof(phone_number)) {
        ESP_LOGW(TAG, "Invalid phone number input");
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(phone_number, number, sizeof(phone_number));
    phone_number[sizeof(phone_number) - 1] = '\0';
    ESP_LOGI(TAG, "Phone number updated: %s", phone_number);
    return ESP_OK;
}

esp_err_t sim4g_gps_is_enabled(bool *enabled) {
    if (!enabled)
        return ESP_ERR_INVALID_ARG;

    char response[64] = {0};
    
    esp_err_t err = sim4g_at_send_by_id(AT_CMD_GPS_STATUS_ID, response, sizeof(response));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "GPS status query failed: %s", esp_err_to_name(err));
        return err;
    }

    *enabled = (strstr(response, "+QGPS: 1") != NULL);
    return ESP_OK;
}

esp_err_t sim4g_gps_update_location(void) {
    esp_err_t result = ESP_FAIL;

    if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        char ts[20] = {0};
        char lat_str[20] = {0};
        char lon_str[20] = {0};

        // Lấy dữ liệu GPS thô dưới dạng chuỗi từ module
        esp_err_t err = sim4g_at_get_location(ts, lat_str, lon_str);
        if (err == ESP_OK) {
            // Chuyển đổi dữ liệu chuỗi thành double
            double latitude = atof(lat_str);
            double longitude = atof(lon_str);

            // Cập nhật dữ liệu đã chuyển đổi vào data_manager
            data_manager_set_gps_location(latitude, longitude);
            
            result = ESP_OK;
        } else {
            ESP_LOGW(TAG, "Failed to get GPS data: %s", esp_err_to_name(err));
        }
        xSemaphoreGive(gps_mutex);
    } else {
        ESP_LOGW(TAG, "Could not acquire GPS mutex");
        result = ESP_ERR_TIMEOUT;
    }

    return result;
}

esp_err_t sim4g_gps_send_fall_alert_sms(const sim4g_gps_data_t *location) {
    if (!location || !location->valid)
        return ESP_ERR_INVALID_ARG;

    char msg[256];
    snprintf(msg, sizeof(msg), "Fall detected!\nLat: %.6f\nLon: %.6f\nTime: %s",
             location->latitude, location->longitude, location->timestamp);

    ESP_LOGI(TAG, "Sending SMS to %s:\n%s", phone_number, msg);

    return sim4g_at_send_sms(phone_number, msg);
}

esp_err_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *data,
                                          void (*callback)(bool success)) {
    if (!data || !data->valid) {
        ESP_LOGE(TAG, "Invalid GPS data for SMS alert");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Cấp phát bộ nhớ cho dữ liệu và copy
    sim4g_gps_data_t *copy = (sim4g_gps_data_t *)malloc(sizeof(sim4g_gps_data_t));
    if (!copy) {
        ESP_LOGE(TAG, "Out of memory for SMS task allocation");
        return ESP_ERR_NO_MEM;
    }
    
    memcpy(copy, data, sizeof(sim4g_gps_data_t));

    if (xTaskCreate(send_sms_task, "sim4g_sms_task", 4096, copy, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create SMS task");
        free(copy);
        return ESP_FAIL;
    }

    return ESP_OK;
}
