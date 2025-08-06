
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "esp_log.h"
#include "esp_err.h"

#include "data_manager.h"
#include "sim4g_at.h"
#include "sim4g_at_cmd.h"
#include "sim4g_gps.h"
#include "data_manager.h"
#include "sdkconfig.h"

#define DEFAULT_PHONE CONFIG_SIM4G_DEFAULT_PHONE
#define MONITORING_INTERVAL_MS 30000 // 30 seconds

static const char *TAG = "SIM4G_GPS";

// Internal state
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[16] = DEFAULT_PHONE;

// Internal task for sending fall alert SMS and MQTT messages
static void send_alert_task(void *param) {
    sim4g_gps_data_t *loc = (sim4g_gps_data_t *)param;

    char msg[256];
    char json_data[256];

    // Create SMS message content based on GPS availability
    if (loc->has_gps_fix) {
        snprintf(msg, sizeof(msg), "Fall detected!\nLat: %.6f\nLon: %.6f\nTime: %s",
                 loc->latitude, loc->longitude, loc->timestamp);
        snprintf(json_data, sizeof(json_data), "{\"event\":\"fall\",\"lat\":%.6f,\"lon\":%.6f,\"time\":\"%s\"}",
                 loc->latitude, loc->longitude, loc->timestamp);
    } else {
        snprintf(msg, sizeof(msg), "Fall detected! GPS data unavailable.");
        snprintf(json_data, sizeof(json_data), "{\"event\":\"fall\",\"message\":\"GPS data unavailable\"}");
    }

    // Send SMS
    ESP_LOGI(TAG, "Sending SMS to %s:\n%s", phone_number, msg);
    esp_err_t err = sim4g_at_send_sms(phone_number, msg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SMS send failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "SMS sent successfully");
    }

    // Send MQTT data (assuming a function named mqtt_publish_json exists)
    // You will need to implement this part if you are using MQTT.
    // esp_err_t mqtt_err = mqtt_publish_json("fall_alerts", json_data);
    // if (mqtt_err != ESP_OK) {
    //     ESP_LOGE(TAG, "MQTT publish failed");
    // } else {
    //     ESP_LOGI(TAG, "MQTT alert published successfully");
    // }

    free(loc); // Free the allocated memory
    vTaskDelete(NULL);
}

// Internal task for periodic MQTT status updates
static void mqtt_monitoring_task(void *pvParameters) {
    while (1) {
        // Get the latest data from the data_manager
        sim4g_gps_data_t current_gps_data;
        data_manager_get_gps_data(&current_gps_data);
        
        char json_data[256];

        // Create the JSON payload for the status update
        if (current_gps_data.has_gps_fix) {
            snprintf(json_data, sizeof(json_data),
                     "{\"status\":\"online\",\"lat\":%.6f,\"lon\":%.6f,\"time\":\"%s\"}",
                     current_gps_data.latitude, current_gps_data.longitude, current_gps_data.timestamp);
        } else {
            snprintf(json_data, sizeof(json_data),
                     "{\"status\":\"online\",\"message\":\"GPS data unavailable\"}");
        }

        // Publish the JSON data to a different MQTT topic, e.g., "device_status"
        ESP_LOGI(TAG, "Publishing status to MQTT:\n%s", json_data);
        // esp_err_t mqtt_err = mqtt_publish_json("device_status", json_data);
        // if (mqtt_err != ESP_OK) {
        //     ESP_LOGE(TAG, "MQTT status publish failed");
        // } else {
        //     ESP_LOGI(TAG, "MQTT status published successfully");
        // }

        vTaskDelay(pdMS_TO_TICKS(MONITORING_INTERVAL_MS));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────


esp_err_t sim4g_gps_init(void) {
    ESP_LOGI(TAG, "Starting SIM4G GPS component initialization...");

    if (!gps_mutex) {
        gps_mutex = xSemaphoreCreateMutex();
        if (!gps_mutex) {
            ESP_LOGE(TAG, "Failed to create GPS mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(TAG, "Calling sim4g_at_init() to set up UART.");
    esp_err_t err = sim4g_at_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SIM4G AT initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // NEW: Wait for the module to register on the cellular network first
    int retry_count = 0;
    const int max_retries = 30; // 30 retries with 1-second delay = 30 seconds
    ESP_LOGI(TAG, "Waiting for cellular network registration...");

    while (retry_count < max_retries) {
        if (sim4g_at_check_network_registration() == ESP_OK) {
            ESP_LOGI(TAG, "Network registration successful after %d seconds.", retry_count);
            break;
        }
        ESP_LOGI(TAG, "Attempt %d/%d: Not registered yet. Retrying in 1 second...", retry_count + 1, max_retries);
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }

    if (retry_count >= max_retries) {
        ESP_LOGE(TAG, "Failed to register on cellular network after %d attempts.", max_retries);
        return ESP_FAIL;
    }
    
    // Now that the network is registered, try to configure and enable GPS.
    ESP_LOGI(TAG, "Calling sim4g_at_configure_gps() to set up autogps.");
    sim4g_at_configure_gps();
    
    ESP_LOGI(TAG, "Calling sim4g_at_enable_gps() to power on GPS.");
    err = sim4g_at_enable_gps();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "GPS enabled successfully");
        // Start the periodic monitoring task ONLY IF GPS init succeeds
        ESP_LOGI(TAG, "Creating MQTT monitoring task.");
        xTaskCreate(mqtt_monitoring_task, "mqtt_monitoring_task", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGE(TAG, "GPS initialization failed: %s", esp_err_to_name(err));
    }
    return err;
}

// ... ( set phone )
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
        char ts[64] = {0};
        char lat_str[64] = {0};
        char lon_str[64] = {0};
        
        sim4g_gps_data_t current_gps_data = {0};

        if (sim4g_at_get_location(ts, lat_str, lon_str) == ESP_OK) {
            current_gps_data.latitude = atof(lat_str);
            current_gps_data.longitude = atof(lon_str);
            strncpy(current_gps_data.timestamp, ts, sizeof(current_gps_data.timestamp) - 1);
            current_gps_data.has_gps_fix = true;
            
            ESP_LOGI(TAG, "GPS data updated: Lat=%.6f Lon=%.6f", current_gps_data.latitude, current_gps_data.longitude);
            result = ESP_OK;
        } else {
            ESP_LOGW(TAG, "Failed to get GPS data.");
            current_gps_data.has_gps_fix = false;
        }
        
        data_manager_set_gps_data(&current_gps_data);
        
        xSemaphoreGive(gps_mutex);
    } else {
        ESP_LOGW(TAG, "Could not acquire GPS mutex");
        result = ESP_ERR_TIMEOUT;
    }

    return result;
}

esp_err_t sim4g_gps_send_fall_alert_async(sim4g_gps_data_t data) {
    sim4g_gps_data_t *copy = (sim4g_gps_data_t *)malloc(sizeof(sim4g_gps_data_t));
    if (!copy) {
        ESP_LOGE(TAG, "Out of memory for SMS task allocation");
        return ESP_ERR_NO_MEM;
    }
    
    memcpy(copy, &data, sizeof(sim4g_gps_data_t));

    if (xTaskCreate(send_alert_task, "sim4g_alert_task", 4096, copy, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create alert task");
        free(copy);
        return ESP_FAIL;
    }

    return ESP_OK;
}
