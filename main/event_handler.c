
#include "event_handler.h"
#include "app_main.h"
#include "buzzer.h"
#include "debugs.h"
#include "fall_logic.h"
#include "led_indicator.h"
#include "sim4g_gps.h"
#include <inttypes.h> // Dùng PRIu32 để log uint32_t và uint8_t an toàn
#define ALERT_DURATION_MS 8000

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

  while (1) {
    if (xQueueReceive(get_event_queue(), &event, portMAX_DELAY)) {
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
 * @brief Start the event handler task for processing fall events
 */
void event_handler_start(void) {
  BaseType_t result =
      xTaskCreate(alert_task, "fall_alert_task", 4096, NULL, 5, NULL);
  if (result != pdPASS) {
    DEBUGS_LOGE("Failed to create fall_alert_task. Restarting...");
    esp_restart();
  }
}
