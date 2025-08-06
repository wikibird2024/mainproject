#include "event_handler.h"
#include "buzzer.h"
#include "led_indicator.h"
#include "sim4g_gps.h"
#include "data_manager.h"
#include "fall_logic.h" // Thêm include fall_logic.h để truy cập hàm reset

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
static TaskHandle_t s_alert_task_handle = NULL;

static void sms_callback(bool success) {
  if (success) {
    ESP_LOGI(TAG, "SMS sent successfully.");
  } else {
    ESP_LOGE(TAG, "Failed to send SMS.");
  }
}

static void alert_task(void *param) {
  system_event_t event;

  ESP_LOGI(TAG, "Event handler task started");

  while (1) {
    if (xQueueReceive(s_event_queue_handle, &event, portMAX_DELAY)) {
      switch (event) {
        case EVENT_FALL_DETECTED: {
          ESP_LOGI(TAG, "Received EVENT_FALL_DETECTED. Triggering alert.");
          
          sim4g_gps_data_t location = {0};
          
          data_manager_get_gps_location(&location.latitude, &location.longitude);
          
          if (location.latitude != 0.0 || location.longitude != 0.0) {
              location.valid = true;
          } else {
              ESP_LOGW(TAG, "Failed to get valid GPS location from data_manager. Sending alert without location.");
          }

          sim4g_gps_send_fall_alert_async(&location, sms_callback);

          buzzer_beep(ALERT_DURATION_MS);
          led_indicator_set_mode(LED_MODE_BLINK_ERROR);

          vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));

          buzzer_stop();
          led_indicator_set_mode(LED_MODE_OFF);

          // BƯỚC MỚI: Hoàn tất vòng lặp phản hồi
          // Sau khi hoàn tất chuỗi cảnh báo, reset trạng thái ngã trong fall_logic
          fall_logic_reset_fall_status();
          ESP_LOGI(TAG, "Alert sequence completed. Fall status has been reset.");

          break;
        }

        default:
          ESP_LOGI(TAG, "Received unknown event: %d. Ignored.", event);
          break;
      }
    }
  }
}

esp_err_t event_handler_init(void) {
  if (s_event_queue_handle != NULL || s_alert_task_handle != NULL) {
    ESP_LOGW(TAG, "Event handler already initialized");
    return ESP_ERR_INVALID_STATE;
  }

  s_event_queue_handle = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(system_event_t));
  if (s_event_queue_handle == NULL) {
    ESP_LOGE(TAG, "Failed to create event queue");
    return ESP_FAIL;
  }

  BaseType_t result = xTaskCreate(alert_task,
                                  "event_handler_task",
                                  EVENT_HANDLER_TASK_STACK_SIZE,
                                  NULL,
                                  EVENT_HANDLER_TASK_PRIORITY,
                                  &s_alert_task_handle);

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
  if (s_alert_task_handle != NULL) {
    vTaskDelete(s_alert_task_handle);
    s_alert_task_handle = NULL;
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
    
    // Logic này vẫn chưa hoàn hảo, nó không đảm bảo 100% không trùng.
    // Với logic mới, fall_logic.c đã tự quản lý trạng thái, nên đoạn code này có thể đơn giản hóa.
    if (event == EVENT_FALL_DETECTED) {
        UBaseType_t count = uxQueueMessagesWaiting(s_event_queue_handle);
        if (count > 0) {
            ESP_LOGW(TAG, "Fall event already in queue. Ignoring.");
            return ESP_OK;
        }
    }
    
    BaseType_t ret = xQueueSend(s_event_queue_handle, &event, pdMS_TO_TICKS(10));
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to send event to queue. Queue full?");
        return ESP_FAIL;
    }
    return ESP_OK;
}
