/**
 * @file fall_logic.c
 * @brief Fall detection logic using MPU6050 sensor.
 *
 * This module provides logic to detect sudden falls based on acceleration data.
 * It runs a FreeRTOS task periodically to analyze sensor data, and sends a
 * fall_event_t to a provided queue when a fall is detected.
 */

#include "fall_logic.h"
#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "mpu6050.h"
#include "sdkconfig.h"

// #define FALL_THRESHOLD CONFIG_FALL_LOGIC_THRESHOLD_G
#define FALL_THRESHOLD ((float)CONFIG_FALL_LOGIC_THRESHOLD_G / 1000.0f)
#define CHECK_INTERVAL_MS CONFIG_FALL_LOGIC_CHECK_INTERVAL_MS
#define FALL_TASK_STACK_SIZE CONFIG_FALL_LOGIC_TASK_STACK_SIZE
#define FALL_TASK_PRIORITY CONFIG_FALL_LOGIC_TASK_PRIORITY
#define MUTEX_TIMEOUT_MS 1000

// Local static references (injected)
static SemaphoreHandle_t s_mutex = NULL;
static QueueHandle_t s_event_queue = NULL;
static bool s_fall_logic_enabled = true;

static bool detect_fall(sensor_data_t data) {
  float total_accel =
      sqrtf(data.accel_x * data.accel_x + data.accel_y * data.accel_y +
            data.accel_z * data.accel_z);
  return total_accel < FALL_THRESHOLD;
}

static void fall_task(void *param) {
  DEBUGS_LOGI("Fall detection task started");

  while (1) {
    if (!s_fall_logic_enabled) {
      vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
      continue;
    }

    sensor_data_t data;

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS))) {
      if (mpu6050_read_data(&data) == ESP_OK) {
        if (detect_fall(data)) {
          float magnitude =
              sqrtf(data.accel_x * data.accel_x + data.accel_y * data.accel_y +
                    data.accel_z * data.accel_z);

          DEBUGS_LOGI("Fall contidion: Accel=(%.2f, %.2f, %.2f), Gyro=(%.2f, "
                      "%.2f, %.2f)",
                      data.accel_x, data.accel_y, data.accel_z, data.gyro_x,
                      data.gyro_y, data.gyro_z);

          fall_event_t event = {
              .timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS,
              .acceleration_x = data.accel_x,
              .acceleration_y = data.accel_y,
              .acceleration_z = data.accel_z,
              .magnitude = magnitude,
              .is_fall_detected = true,
              .confidence = 90 // Placeholder confidence
          };

          xQueueSend(s_event_queue, &event, 0);
        }
      } else {
        DEBUGS_LOGW("Failed to read MPU6050 data");
      }

      xSemaphoreGive(s_mutex);
    } else {
      DEBUGS_LOGW("Failed to acquire mutex");
    }

    vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
  }
}

esp_err_t fall_logic_init(SemaphoreHandle_t mutex, QueueHandle_t event_queue) {
  if (mutex == NULL || event_queue == NULL) {
    DEBUGS_LOGE("Invalid mutex or event queue");
    return ESP_ERR_INVALID_ARG;
  }

  s_mutex = mutex;
  s_event_queue = event_queue;

  DEBUGS_LOGI("Fall logic initialized");
  return ESP_OK;
}

void fall_logic_start(void) {
  xTaskCreate(fall_task, "fall_task", FALL_TASK_STACK_SIZE, NULL,
              FALL_TASK_PRIORITY, NULL);
}

void fall_logic_enable(void) {
  s_fall_logic_enabled = true;
  DEBUGS_LOGI("Fall logic enabled");
}

void fall_logic_disable(void) {
  s_fall_logic_enabled = false;
  DEBUGS_LOGI("Fall logic disabled");
}

bool fall_logic_is_enabled(void) { return s_fall_logic_enabled; }
