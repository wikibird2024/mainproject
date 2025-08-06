#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"

#include "data_manager.h"
#include "sim4g_gps.h" // For sim4g_gps_data_t
#include "event_handler.h" // Still needed for system_event_t

static const char *TAG = "DATA_MANAGER";
static device_state_t s_device_state;
static SemaphoreHandle_t s_data_mutex;

// ─────────────────────────────────────────────────────────────────────────────
// Initialization and Deinitialization
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
// GET Functions
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

esp_err_t data_manager_get_gps_data(sim4g_gps_data_t *data) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(data, &s_device_state.gps_data, sizeof(sim4g_gps_data_t));
        xSemaphoreGive(s_data_mutex);
        return ESP_OK;
    }
    return ESP_FAIL;
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
// SET Functions
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t data_manager_set_fall_status(bool state) {
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        s_device_state.fall_detected = state;
        s_device_state.timestamp_ms = esp_timer_get_time() / 1000;
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "Fall status updated to: %s", state ? "true" : "false");

        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t data_manager_set_gps_data(const sim4g_gps_data_t *data) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_data_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&s_device_state.gps_data, data, sizeof(sim4g_gps_data_t));
        s_device_state.timestamp_ms = esp_timer_get_time() / 1000;
        xSemaphoreGive(s_data_mutex);
        
        ESP_LOGI(TAG, "GPS data updated: has_fix=%s", data->has_gps_fix ? "true" : "false");
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
