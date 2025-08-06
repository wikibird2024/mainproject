#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "data_manager.h"
#include "esp_log.h"

static const char *TAG = "DATA_MANAGER";

// Biến dữ liệu chính của thiết bị
static device_data_t s_device_data;
// Mutex để bảo vệ s_device_data khỏi các truy cập đồng thời
static SemaphoreHandle_t s_data_mutex;

esp_err_t data_manager_init(void) {
    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }
    
    // Khởi tạo giá trị mặc định
    memset(&s_device_data, 0, sizeof(device_data_t));
    s_device_data.fall_detected = false;
    
    ESP_LOGI(TAG, "Data Manager initialized successfully");
    return ESP_OK;
}

esp_err_t data_manager_get_data(device_data_t *data) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(data, &s_device_data, sizeof(device_data_t));
        xSemaphoreGive(s_data_mutex);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_fall_status(bool state) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_data.fall_detected = state;
        xSemaphoreGive(s_data_mutex);
        
        ESP_LOGI(TAG, "Fall status updated to: %s", state ? "true" : "false");
        // Có thể phát ra sự kiện tại đây nếu cần
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_gps_location(double latitude, double longitude) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_data.latitude = latitude;
        s_device_data.longitude = longitude;
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "GPS location updated: Lat=%.6f, Lon=%.6f", latitude, longitude);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_device_id(const char *device_id) {
    if (device_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        strncpy(s_device_data.device_id, device_id, sizeof(s_device_data.device_id) - 1);
        s_device_data.device_id[sizeof(s_device_data.device_id) - 1] = '\0';
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "Device ID set to: %s", device_id);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_update_timestamp(void) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(s_device_data.timestamp, sizeof(s_device_data.timestamp), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
        xSemaphoreGive(s_data_mutex);
        
        ESP_LOGI(TAG, "Timestamp updated: %s", s_device_data.timestamp);
        return ESP_OK;
    }
    return ESP_FAIL;
}
