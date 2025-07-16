
/**
 * @file buzzer.c
 * @brief Driver điều khiển buzzer cho ESP32 (Active hoặc Passive)
 */

#include "buzzer.h"
#include "buzzer_config.h"
#include "comm.h"
#include "debugs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
  int duration_ms; // < 0: ON vô hạn, 0: OFF, > 0: bật duration_ms rồi tắt
} buzzer_cmd_t;

#define BUZZER_QUEUE_LEN 5
static QueueHandle_t buzzer_queue = NULL;

static void buzzer_hw_on(void) {
#if BUZZER_USE_PASSIVE
  comm_pwm_set_duty_cycle(BUZZER_PWM_DUTY);
#else
  comm_gpio_led_set(1);
#endif
}

static void buzzer_hw_off(void) {
#if BUZZER_USE_PASSIVE
  comm_pwm_stop();
#else
  comm_gpio_led_set(0);
#endif
}

static esp_err_t buzzer_hw_init(void) {
#if BUZZER_USE_PASSIVE
  return comm_pwm_init(BUZZER_PWM_PIN, BUZZER_PWM_FREQ_HZ);
#else
  esp_err_t err = comm_gpio_init(BUZZER_GPIO, -1);
  if (err == ESP_OK) {
    comm_gpio_led_set(0);
  }
  return err;
#endif
}

static void buzzer_task(void *arg) {
  buzzer_cmd_t cmd;
  while (1) {
    if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY)) {
      if (cmd.duration_ms == 0) {
        buzzer_hw_off();
      } else {
        buzzer_hw_on();
        if (cmd.duration_ms > 0) {
          vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));
          buzzer_hw_off();
        }
      }
    }
  }
}

esp_err_t buzzer_init(void) {
  esp_err_t err = buzzer_hw_init();
  if (err != ESP_OK) {
    DEBUGS_LOGE("Buzzer HW init failed: %s", esp_err_to_name(err));
    return err;
  }

  buzzer_queue = xQueueCreate(BUZZER_QUEUE_LEN, sizeof(buzzer_cmd_t));
  if (buzzer_queue == NULL) {
    DEBUGS_LOGE("Buzzer queue create failed");
    return ESP_FAIL;
  }

  if (xTaskCreatePinnedToCore(buzzer_task, "buzzer_task", 1024, NULL, 5, NULL,
                              1) != pdPASS) {
    vQueueDelete(buzzer_queue);
    buzzer_queue = NULL;
    DEBUGS_LOGE("Buzzer task create failed");
    return ESP_FAIL;
  }

  DEBUGS_LOGI("Buzzer driver initialized.");
  return ESP_OK;
}

esp_err_t buzzer_beep(int duration_ms) {
  if (buzzer_queue == NULL)
    return ESP_FAIL;
  buzzer_cmd_t cmd = {.duration_ms = duration_ms};
  if (xQueueSend(buzzer_queue, &cmd, 0) != pdPASS) {
    DEBUGS_LOGW("Buzzer queue full, overwriting...");
    xQueueOverwrite(buzzer_queue, &cmd);
  }
  return ESP_OK;
}

esp_err_t buzzer_stop(void) { return buzzer_beep(0); }
