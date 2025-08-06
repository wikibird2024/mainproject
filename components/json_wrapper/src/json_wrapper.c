#include <stdio.h> // Cần thiết cho sprintf
#include <string.h> // Cần thiết cho strlen, v.v.

#include "json_wrapper.h"
#include "data_manager.h" // Sử dụng module mới
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "JSON_WRAPPER";

char *json_wrapper_create_payload(void) {
    // 1. Lấy dữ liệu từ Data Manager
    device_state_t data;
    esp_err_t err = data_manager_get_device_state(&data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device data from Data Manager: %d", err);
        return NULL;
    }

    // 2. Tạo đối tượng JSON
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return NULL;
    }

    // 3. Xử lý và thêm timestamp dạng chuỗi vào đối tượng JSON
    char timestamp_str[20];
    sprintf(timestamp_str, "%lu", (unsigned long)data.timestamp_ms);
    cJSON_AddStringToObject(root, "timestamp", timestamp_str);

    // 4. Thêm các trường dữ liệu khác vào đối tượng JSON
    cJSON_AddStringToObject(root, "device_id", data.device_id);
    cJSON_AddBoolToObject(root, "fall_detected", data.fall_detected);
    cJSON_AddNumberToObject(root, "latitude", data.latitude);
    cJSON_AddNumberToObject(root, "longitude", data.longitude);

    // 5. In đối tượng JSON ra chuỗi
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON string");
        cJSON_Delete(root);
        return NULL;
    }

    // 6. Giải phóng bộ nhớ của cJSON
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Created JSON payload: %s", json_str);
    return json_str; // Chuỗi này cần được giải phóng bởi caller
}
