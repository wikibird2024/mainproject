#include "event_handler.h"
#include "buzzer.h"
#include "data_manager.h"
#include "fall_logic.h"
#include "led_indicator.h"
#include "sim4g_gps.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define ALERT_DURATION_MS 8000
#define EVENT_QUEUE_LENGTH 10
#define EVENT_HANDLER_TASK_STACK_SIZE 4096
#define EVENT_HANDLER_TASK_PRIORITY 5

static const char *TAG = "EVENT_HANDLER";

static QueueHandle_t s_event_queue_handle = NULL;
static TaskHandle_t s_event_handler_task_handle = NULL;

/**
 * @brief A short-lived task to handle the immediate, time-consuming alert
 * sequence.
 * * This task will run the buzzer and LED for the specified duration and then
 * clean up. This prevents the main event_handler_task from being blocked.
 */
static void alert_sequence_task(void *param) {
  ESP_LOGI(TAG, "Alert sequence task started.");

  buzzer_beep(ALERT_DURATION_MS);
  led_indicator_set_mode(LED_MODE_BLINK_ERROR);

  vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));

  buzzer_stop();
  led_indicator_set_mode(LED_MODE_OFF);

  // Once the alert sequence is complete, reset the fall status
  fall_logic_reset_fall_status();
  ESP_LOGI(TAG, "Alert sequence completed. Fall status has been reset.");

  vTaskDelete(NULL);
}

static void event_handler_task(void *param) {
  system_event_t event;

  ESP_LOGI(TAG, "Event handler task started");

  while (1) {
    if (xQueueReceive(s_event_queue_handle, &event, portMAX_DELAY)) {
      switch (event) {
      case EVENT_FALL_DETECTED: {
        ESP_LOGI(TAG, "Received EVENT_FALL_DETECTED. Triggering alert.");

        // Get the latest GPS data from the data_manager
        gps_data_t location = {0};
        data_manager_get_gps_data(&location);

        // Start the SMS/MQTT task (non-blocking)
        sim4g_gps_start_fall_alert(&location);

        // Start the blocking alert sequence in a separate task
        xTaskCreate(alert_sequence_task, "alert_seq_task", 2048, NULL, 4, NULL);

        break;
      }
      case EVENT_WIFI_CONNECTED:
        ESP_LOGI(TAG, "Received EVENT_WIFI_CONNECTED.");
        // Your non-blocking logic for WiFi connected, e.g., MQTT reconnect.
        break;

      case EVENT_MQTT_CONNECTED:
        ESP_LOGI(TAG, "Received EVENT_MQTT_CONNECTED.");
        // Your non-blocking logic for MQTT connected.
        break;

      default:
        ESP_LOGI(TAG, "Received unknown event: %d. Ignored.", event);
        break;
      }
    }
  }
}

// (event_handler_init, event_handler_deinit, event_handler_send_event)
esp_err_t event_handler_init(void) {
  if (s_event_queue_handle != NULL || s_event_handler_task_handle != NULL) {
    ESP_LOGW(TAG, "Event handler already initialized");
    return ESP_ERR_INVALID_STATE;
  }

  s_event_queue_handle =
      xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(system_event_t));
  if (s_event_queue_handle == NULL) {
    ESP_LOGE(TAG, "Failed to create event queue");
    return ESP_FAIL;
  }

  BaseType_t result = xTaskCreate(
      event_handler_task, "event_handler_task", EVENT_HANDLER_TASK_STACK_SIZE,
      NULL, EVENT_HANDLER_TASK_PRIORITY, &s_event_handler_task_handle);

  if (result != pdPASS) {
    ESP_LOGE(TAG, "Failed to create event handler task");
    vQueueDelete(s_event_queue_handle);
    s_event_queue_handle = NULL;
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Event handler initialized successfully");
  return ESP_OK;
}

esp_err_t event_handler_deinit(void) {
  if (s_event_handler_task_handle != NULL) {
    vTaskDelete(s_event_handler_task_handle);
    s_event_handler_task_handle = NULL;
  }

  if (s_event_queue_handle != NULL) {
    vQueueDelete(s_event_queue_handle);
    s_event_queue_handle = NULL;
  }

  ESP_LOGI(TAG, "Event handler deinitialized");
  return ESP_OK;
}

esp_err_t event_handler_send_event(system_event_t event) {
  if (s_event_queue_handle == NULL) {
    ESP_LOGE(TAG, "Event handler not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  BaseType_t ret = xQueueSend(s_event_queue_handle, &event, pdMS_TO_TICKS(10));
  if (ret != pdPASS) {
    ESP_LOGE(TAG, "Failed to send event to queue. Queue full?");
    return ESP_FAIL;
  }
  return ESP_OK;
}
