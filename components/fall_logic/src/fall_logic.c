#include "fall_logic.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "mpu6050.h"
#include "data_manager.h"
#include "event_handler.h"

static const char *TAG = "FALL_LOGIC";

#define FALL_THRESHOLD ((float)CONFIG_FALL_LOGIC_THRESHOLD_G / 1000.0f)
#define CHECK_INTERVAL_MS CONFIG_FALL_LOGIC_CHECK_INTERVAL_MS
#define FALL_TASK_STACK_SIZE CONFIG_FALL_LOGIC_TASK_STACK_SIZE
#define FALL_TASK_PRIORITY CONFIG_FALL_LOGIC_TASK_PRIORITY

// Biến nội bộ quản lý trạng thái
static bool s_fall_logic_enabled = true;
// Biến cờ mới để quản lý trạng thái ngã đã được phát hiện hay chưa
static bool s_fall_detected = false; 

/**
 * @brief Simple algorithm to detect a fall based on acceleration magnitude.
 * @param data Sensor data from MPU6050.
 * @return true if a fall condition is detected, false otherwise.
 */
static bool detect_fall(sensor_data_t data) {
  float total_accel = sqrtf(data.accel_x * data.accel_x + data.accel_y * data.accel_y + data.accel_z * data.accel_z);
  return total_accel < FALL_THRESHOLD;
}

/**
 * @brief FreeRTOS task for continuous fall detection.
 */
static void fall_task(void *param) {
  ESP_LOGI(TAG, "Fall detection task started");

  while (1) {
    if (!s_fall_logic_enabled) {
      vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
      continue;
    }

    sensor_data_t data;

    if (mpu6050_read_data(&data) == ESP_OK) {
      // Logic mới: Chỉ xử lý nếu chưa có cú ngã nào đang được xử lý
      if (detect_fall(data) && !s_fall_detected) {
        ESP_LOGW(TAG, "FALL DETECTED! Accel=(%.2f, %.2f, %.2f)",
                 data.accel_x, data.accel_y, data.accel_z);
                
        // Gui su kien cho even handler
        event_handler_send_event(EVENT_FALL_DETECTED); 
                
        // Đặt cờ ngã đã được phát hiện
        s_fall_detected = true;
        
      }
    } else {
      ESP_LOGE(TAG, "Failed to read MPU6050 data");
    }

    vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
  }
}

// Hàm khởi tạo module
esp_err_t fall_logic_init(void) {
  ESP_LOGI(TAG, "Fall logic initialized");
  return ESP_OK;
}

// Bắt đầu task phát hiện ngã
esp_err_t fall_logic_start(void) {
  BaseType_t result = xTaskCreate(fall_task, "fall_task", FALL_TASK_STACK_SIZE,
                                  NULL, FALL_TASK_PRIORITY, NULL);

  if (result != pdPASS) {
    ESP_LOGE(TAG, "Failed to create fall_task");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Fall logic task created successfully");
  return ESP_OK;
}

// Các hàm enable/disable và kiểm tra trạng thái
esp_err_t fall_logic_enable(void) {
  s_fall_logic_enabled = true;
  ESP_LOGI(TAG, "Fall logic enabled");
  return ESP_OK;
}

esp_err_t fall_logic_disable(void) {
  s_fall_logic_enabled = false;
  ESP_LOGI(TAG, "Fall logic disabled");
  return ESP_OK;
}

bool fall_logic_is_enabled(void) {
  return s_fall_logic_enabled;
}

// Hàm mới để reset trạng thái ngã, được gọi từ event_handler
esp_err_t fall_logic_reset_fall_status(void) {
    if (s_fall_detected) {
        s_fall_detected = false;
        ESP_LOGI(TAG, "Fall status has been reset.");
    }
    return ESP_OK;
}
