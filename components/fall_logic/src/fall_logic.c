#include "fall_logic.h"
#include "data_manager.h"
#include "esp_log.h"
#include "event_handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "math.h"
#include "mpu6050.h"

static const char *TAG = "FALL_LOGIC";

#define FALL_THRESHOLD ((float)CONFIG_FALL_LOGIC_THRESHOLD_G / 1000.0f)
#define CHECK_INTERVAL_MS CONFIG_FALL_LOGIC_CHECK_INTERVAL_MS
#define FALL_TASK_STACK_SIZE CONFIG_FALL_LOGIC_TASK_STACK_SIZE
#define FALL_TASK_PRIORITY CONFIG_FALL_LOGIC_TASK_PRIORITY

// Internal state variables
static bool s_fall_logic_enabled = true;
// Flag to manage if a fall has been detected and is being processed
static bool s_fall_detected = false;
// Mutex to protect the s_fall_detected flag from race conditions
static portMUX_TYPE s_fall_detected_mux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Simple algorithm to detect a fall based on acceleration magnitude.
 * @param data Sensor data from MPU6050.
 * @return true if a fall condition is detected, false otherwise.
 */
static bool detect_fall(sensor_data_t data) {
  float total_accel =
      sqrtf(data.accel_x * data.accel_x + data.accel_y * data.accel_y +
            data.accel_z * data.accel_z);
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
      if (detect_fall(data)) {
        // Use a critical section to safely check and set the flag
        taskENTER_CRITICAL(&s_fall_detected_mux);
        if (!s_fall_detected) {
          ESP_LOGW(TAG, "FALL DETECTED! Accel=(%.2f, %.2f, %.2f)", data.accel_x,
                   data.accel_y, data.accel_z);

          // Send event to the event handler
          event_handler_send_event(EVENT_FALL_DETECTED);

          // Set the flag to prevent repeated events until the alert is handled
          s_fall_detected = true;
        }
        taskEXIT_CRITICAL(&s_fall_detected_mux);
      }
    } else {
      ESP_LOGE(TAG, "Failed to read MPU6050 data");
    }

    vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
  }
}

// Module initialization function
esp_err_t fall_logic_init(void) {
  ESP_LOGI(TAG, "Fall logic initialized");
  return ESP_OK;
}

// Starts the fall detection task
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

// Enable/disable and status check functions
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

bool fall_logic_is_enabled(void) { return s_fall_logic_enabled; }

// New function to reset the fall status, to be called from event_handler
esp_err_t fall_logic_reset_fall_status(void) {
  // Use a critical section to safely reset the flag
  taskENTER_CRITICAL(&s_fall_detected_mux);
  s_fall_detected = false;
  ESP_LOGI(TAG, "Fall status has been reset.");
  taskEXIT_CRITICAL(&s_fall_detected_mux);
  return ESP_OK;
}
