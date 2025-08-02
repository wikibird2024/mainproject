
// buzzer.c - Active buzzer component using ESP-IDF (LEDC PWM)

#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_GPIO CONFIG_BUZZER_GPIO
#define BUZZER_LEDC_TIMER LEDC_TIMER_0
#define BUZZER_LEDC_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_DUTY (CONFIG_BUZZER_DUTY) // 0-8191
#define BUZZER_FREQ_HZ (CONFIG_BUZZER_FREQ)

static const char *TAG = "buzzer";

static bool buzzer_initialized = false;

esp_err_t buzzer_init(void) {
  if (buzzer_initialized)
    return ESP_OK;

  ledc_timer_config_t timer_conf = {.speed_mode = BUZZER_LEDC_MODE,
                                    .timer_num = BUZZER_LEDC_TIMER,
                                    .duty_resolution = LEDC_TIMER_13_BIT,
                                    .freq_hz = BUZZER_FREQ_HZ,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

  ledc_channel_config_t channel_conf = {.gpio_num = BUZZER_GPIO,
                                        .speed_mode = BUZZER_LEDC_MODE,
                                        .channel = BUZZER_LEDC_CHANNEL,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = BUZZER_LEDC_TIMER,
                                        .duty = 0,
                                        .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

  buzzer_initialized = true;
  ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_GPIO);
  return ESP_OK;
}

esp_err_t buzzer_beep(uint32_t duration_ms) {
  if (!buzzer_initialized)
    return ESP_ERR_INVALID_STATE;

  ESP_ERROR_CHECK(
      ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY));
  ESP_ERROR_CHECK(ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL));

  vTaskDelay(pdMS_TO_TICKS(duration_ms));

  return buzzer_stop();
}

esp_err_t buzzer_stop(void) {
  if (!buzzer_initialized)
    return ESP_ERR_INVALID_STATE;

  ESP_ERROR_CHECK(ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0));
  ESP_ERROR_CHECK(ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL));

  ESP_LOGD(TAG, "Buzzer stopped");
  return ESP_OK;
}
