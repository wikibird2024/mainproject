#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "data_manager.h"
#include "event_handler.h" // Vẫn cần để khai báo system_event_t
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h> // Cần cho memset và memcpy
#include <stdio.h>  // Cần cho snprintf
#include "esp_random.h"

static const char *TAG = "DATA_MANAGER";
static device_state_t s_device_state;
static SemaphoreHandle_t s_data_mutex;

// ─────────────────────────────────────────────────────────────────────────────
// Khởi tạo và giải phóng
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t data_manager_init(void) {
    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_FAIL;
    }

    memset(&s_device_state, 0, sizeof(device_state_t));
    
    snprintf(s_device_state.device_id, sizeof(s_device_state.device_id), "ESP32_DEV_%06lX", (unsigned long)esp_random() % 0xFFFFFF);
    
    ESP_LOGI(TAG, "Data Manager initialized successfully with ID: %s", s_device_state.device_id);
    return ESP_OK;
}

void data_manager_deinit(void) {
    if (s_data_mutex != NULL) {
        vSemaphoreDelete(s_data_mutex);
        s_data_mutex = NULL;
        ESP_LOGI(TAG, "Data Manager deinitialized");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Các hàm GET
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t data_manager_get_device_state(device_state_t *state) {
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(state, &s_device_state, sizeof(device_state_t));
        xSemaphoreGive(s_data_mutex);
        return ESP_OK;
    }
    return ESP_FAIL;
}

bool data_manager_get_fall_status(void) {
    bool status = false;
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        status = s_device_state.fall_detected;
        xSemaphoreGive(s_data_mutex);
    }
    return status;
}

void data_manager_get_gps_location(double *latitude, double *longitude) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        *latitude = s_device_state.latitude;
        *longitude = s_device_state.longitude;
        xSemaphoreGive(s_data_mutex);
    }
}

bool data_manager_get_wifi_status(void) {
    bool status = false;
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        status = s_device_state.wifi_connected;
        xSemaphoreGive(s_data_mutex);
    }
    return status;
}

bool data_manager_get_mqtt_status(void) {
    bool status = false;
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        status = s_device_state.mqtt_connected;
        xSemaphoreGive(s_data_mutex);
    }
    return status;
}

esp_err_t data_manager_get_device_id(char* id_buffer, size_t buffer_size) {
    if (id_buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        strncpy(id_buffer, s_device_state.device_id, buffer_size - 1);
        id_buffer[buffer_size - 1] = '\0';
        xSemaphoreGive(s_data_mutex);
        return ESP_OK;
    }
    return ESP_FAIL;
}


// ─────────────────────────────────────────────────────────────────────────────
// Các hàm SET
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t data_manager_set_fall_status(bool state) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_state.fall_detected = state;
        s_device_state.timestamp_ms = esp_timer_get_time() / 1000;
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "Fall status updated to: %s", state ? "true" : "false");

        // Đã loại bỏ logic phát sự kiện ở đây để chuyển trách nhiệm cho fall_logic.c
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_gps_location(double latitude, double longitude) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_state.latitude = latitude;
        s_device_state.longitude = longitude;
        s_device_state.timestamp_ms = esp_timer_get_time() / 1000;
        xSemaphoreGive(s_data_mutex);
        
        ESP_LOGI(TAG, "GPS location updated: Lat=%.6f, Lon=%.6f", latitude, longitude);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_wifi_status(bool connected) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_state.wifi_connected = connected;
        xSemaphoreGive(s_data_mutex);
        
        ESP_LOGI(TAG, "WiFi status updated to: %s", connected ? "connected" : "disconnected");
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_mqtt_status(bool connected) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_state.mqtt_connected = connected;
        xSemaphoreGive(s_data_mutex);
        
        ESP_LOGI(TAG, "MQTT status updated to: %s", connected ? "connected" : "disconnected");
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_sim_status(bool registered) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_state.sim_registered = registered;
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "SIM status updated to: %s", registered ? "registered" : "not registered");
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_device_id(const char* id) {
    if (id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        strncpy(s_device_state.device_id, id, sizeof(s_device_state.device_id) - 1);
        s_device_state.device_id[sizeof(s_device_state.device_id) - 1] = '\0';
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "Device ID set to: %s", id);
        return ESP_OK;
    }
    return ESP_FAIL;
}
