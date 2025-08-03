#include "event_handler.h"
#include "buzzer.h"
#include "debugs.h"
#include "fall_logic.h"
#include "led_indicator.h"
#include "sim4g_gps.h"
#include <inttypes.h> // Dùng PRIu32 để log uint32_t và uint8_t an toàn

#define ALERT_DURATION_MS 8000

// Static variables
static QueueHandle_t event_queue_handle = NULL;
static TaskHandle_t alert_task_handle = NULL;

/**
 * @brief Callback for SMS sending result
 */
static void sms_callback(bool success) {
  if (success) {
    DEBUGS_LOGI("SMS sent successfully.");
  } else {
    DEBUGS_LOGE("Failed to send SMS.");
  }
}

/**
 * @brief Task to handle fall events received from fall_logic
 */
static void alert_task(void *param) {
  fall_event_t event;
  QueueHandle_t queue = (QueueHandle_t)param;

  if (queue == NULL) {
    DEBUGS_LOGE("Alert task received NULL queue handle");
    vTaskDelete(NULL);
    return;
  }

  while (1) {
    if (xQueueReceive(queue, &event, portMAX_DELAY)) {
      DEBUGS_LOGI(
          "Fall event received: timestamp=%" PRIu32
          ", accel=(%.2f, %.2f, %.2f), magnitude=%.2f, confidence=%" PRIu8 "%%",
          event.timestamp, event.acceleration_x, event.acceleration_y,
          event.acceleration_z, event.magnitude, event.confidence);

      if (event.is_fall_detected) {
        // Step 1: Get GPS location
        sim4g_gps_data_t location = sim4g_gps_get_location();

        // Step 2: Send alert SMS
        sim4g_gps_send_fall_alert_async(&location, sms_callback);

        // Step 3: Trigger buzzer and LED
        buzzer_beep(ALERT_DURATION_MS);
        led_indicator_set_mode(LED_MODE_BLINK_ERROR);

        // Step 4: Wait
        vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));

        // Step 5: Stop alert
        buzzer_stop();
        led_indicator_set_mode(LED_MODE_OFF);
      } else {
        DEBUGS_LOGW("Received non-fall event. Ignored.");
      }
    }
  }
}

/**
 * @brief Initialize the event handler with provided queue
 * @param queue Queue handle for receiving fall events
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if queue is NULL, ESP_FAIL if
 * task creation fails
 */
esp_err_t event_handler_init(QueueHandle_t queue) {
  // Validate input parameters
  if (queue == NULL) {
    DEBUGS_LOGE("Queue handle cannot be NULL");
    return ESP_ERR_INVALID_ARG;
  }

  // Check if already initialized
  if (event_queue_handle != NULL || alert_task_handle != NULL) {
    DEBUGS_LOGW("Event handler already initialized");
    return ESP_ERR_INVALID_STATE;
  }

  // Store queue handle
  event_queue_handle = queue;

  // Create alert task
  BaseType_t result =
      xTaskCreate(alert_task,        // Task function
                  "fall_alert_task", // Task name
                  4096,              // Stack size
                  (void *)queue,     // Task parameter (queue handle)
                  5,                 // Task priority
                  &alert_task_handle // Task handle
      );

  if (result != pdPASS) {
    DEBUGS_LOGE("Failed to create fall_alert_task");
    event_queue_handle = NULL;
    return ESP_FAIL;
  }

  DEBUGS_LOGI("Event handler initialized successfully");
  return ESP_OK;
}

/**
 * @brief Deinitialize the event handler
 * @return ESP_OK on success
 */
esp_err_t event_handler_deinit(void) {
  // Delete task if exists
  if (alert_task_handle != NULL) {
    vTaskDelete(alert_task_handle);
    alert_task_handle = NULL;
  }

  // Clear queue handle
  event_queue_handle = NULL;

  DEBUGS_LOGI("Event handler deinitialized");
  return ESP_OK;
}

/**
 * @brief Check if event handler is initialized
 * @return true if initialized, false otherwise
 */
bool event_handler_is_initialized(void) {
  return (event_queue_handle != NULL && alert_task_handle != NULL);
}

/**
 * @brief Get the queue handle (for backward compatibility or debugging)
 * @return Queue handle or NULL if not initialized
 */
QueueHandle_t event_handler_get_queue(void) { return event_queue_handle; }
