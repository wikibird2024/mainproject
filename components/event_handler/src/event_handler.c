#include "event_handler.h"
#include "buzzer.h"
#include "led_indicator.h"
#include "sim4g_gps.h"
#include "data_manager.h"
#include "fall_logic.h" 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#define ALERT_DURATION_MS 8000
#define EVENT_QUEUE_LENGTH 10
#define EVENT_HANDLER_TASK_STACK_SIZE 4096
#define EVENT_HANDLER_TASK_PRIORITY 5

static const char *TAG = "EVENT_HANDLER";

static QueueHandle_t s_event_queue_handle = NULL;
static TaskHandle_t s_event_handler_task_handle = NULL;

// The old sms_callback is no longer needed as the new sim4g_gps_send_fall_alert_async
// does not require a callback and handles the logic internally.

static void event_handler_task(void *param) {
  system_event_t event;

  ESP_LOGI(TAG, "Event handler task started");

  while (1) {
    if (xQueueReceive(s_event_queue_handle, &event, portMAX_DELAY)) {
      switch (event) {
        case EVENT_FALL_DETECTED: {
          ESP_LOGI(TAG, "Received EVENT_FALL_DETECTED. Triggering alert.");
          
          // Get the latest GPS data from the refactored data_manager
          sim4g_gps_data_t location = {0};
          data_manager_get_gps_data(&location);
          
          // Send the alert asynchronously. The new function handles the
          // SMS and MQTT logic, and it correctly uses the has_gps_fix flag.
          sim4g_gps_send_fall_alert_async(location);

          buzzer_beep(ALERT_DURATION_MS);
          led_indicator_set_mode(LED_MODE_BLINK_ERROR);

          vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));

          buzzer_stop();
          led_indicator_set_mode(LED_MODE_OFF);

          // Once the alert sequence is complete, reset the fall status
          fall_logic_reset_fall_status();
          ESP_LOGI(TAG, "Alert sequence completed. Fall status has been reset.");

          break;
        }
        case EVENT_WIFI_CONNECTED:
            ESP_LOGI(TAG, "Received EVENT_WIFI_CONNECTED.");
            // Add your logic for WiFi connected, e.g., MQTT reconnect.
            break;
        
        case EVENT_MQTT_CONNECTED:
            ESP_LOGI(TAG, "Received EVENT_MQTT_CONNECTED.");
            // Add your logic for MQTT connected.
            break;

        default:
          ESP_LOGI(TAG, "Received unknown event: %d. Ignored.", event);
          break;
      }
    }
  }
}

esp_err_t event_handler_init(void) {
  if (s_event_queue_handle != NULL || s_event_handler_task_handle != NULL) {
    ESP_LOGW(TAG, "Event handler already initialized");
    return ESP_ERR_INVALID_STATE;
  }

  s_event_queue_handle = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(system_event_t));
  if (s_event_queue_handle == NULL) {
    ESP_LOGE(TAG, "Failed to create event queue");
    return ESP_FAIL;
  }

  BaseType_t result = xTaskCreate(event_handler_task,
                                  "event_handler_task",
                                  EVENT_HANDLER_TASK_STACK_SIZE,
                                  NULL,
                                  EVENT_HANDLER_TASK_PRIORITY,
                                  &s_event_handler_task_handle);

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
