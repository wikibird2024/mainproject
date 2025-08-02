/**
 * @file led_indicator.c
 * @brief Implementation of LED Indicator control with various blinking modes.
 */

#include "led_indicator.h"
#include "comm.h"
#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define LED_TASK_STACK_SIZE CONFIG_LED_INDICATOR_TASK_STACK_SIZE
#define LED_TASK_PRIORITY CONFIG_LED_INDICATOR_TASK_PRIORITY

#define FAST_BLINK_PERIOD_MS 200
#define SLOW_BLINK_PERIOD_MS 1000
#define ERROR_BLINK_ON_MS 150
#define ERROR_BLINK_OFF_MS 150
#define ERROR_BLINK_PAUSE_MS 700

static TaskHandle_t led_task_handle = NULL;
static led_mode_t current_mode = LED_MODE_OFF;
static bool led_initialized = false;

static const int led_gpio = CONFIG_LED_INDICATOR_GPIO;
static const bool led_active_high = CONFIG_LED_INDICATOR_ACTIVE_HIGH;

static portMUX_TYPE led_mux = portMUX_INITIALIZER_UNLOCKED;

// Internal: set the LED level using comm abstraction
static inline void led_set(bool on) {
  comm_gpio_led_set(led_active_high ? on : !on);
}

// Internal: safely get current mode
led_mode_t led_indicator_get_mode(void) {
  portENTER_CRITICAL(&led_mux);
  led_mode_t mode = current_mode;
  portEXIT_CRITICAL(&led_mux);
  return mode;
}

// Internal: main LED control task
static void led_task(void *arg) {
  while (1) {
    led_mode_t mode;

    portENTER_CRITICAL(&led_mux);
    mode = current_mode;
    portEXIT_CRITICAL(&led_mux);

    switch (mode) {
    case LED_MODE_OFF:
      led_set(false);
      vTaskDelay(pdMS_TO_TICKS(100));
      break;

    case LED_MODE_ON:
      led_set(true);
      vTaskDelay(pdMS_TO_TICKS(100));
      break;

    case LED_MODE_BLINK_FAST:
      led_set(true);
      vTaskDelay(pdMS_TO_TICKS(FAST_BLINK_PERIOD_MS / 2));
      led_set(false);
      vTaskDelay(pdMS_TO_TICKS(FAST_BLINK_PERIOD_MS / 2));
      break;

    case LED_MODE_BLINK_SLOW:
      led_set(true);
      vTaskDelay(pdMS_TO_TICKS(SLOW_BLINK_PERIOD_MS / 2));
      led_set(false);
      vTaskDelay(pdMS_TO_TICKS(SLOW_BLINK_PERIOD_MS / 2));
      break;

    case LED_MODE_BLINK_ERROR:
      for (int i = 0; i < 2; ++i) {
        led_set(true);
        vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_ON_MS));
        led_set(false);
        vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_OFF_MS));
      }
      vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_PAUSE_MS));
      break;

    default:
      led_set(false);
      vTaskDelay(pdMS_TO_TICKS(500));
      break;
    }
  }
}

esp_err_t led_indicator_init(void) {
#if CONFIG_LED_INDICATOR_ENABLE
  if (led_initialized) {
    DEBUGS_LOGW("LED already initialized.");
    return ESP_OK;
  }

  esp_err_t err = comm_gpio_init(led_gpio, -1);
  if (err != ESP_OK) {
    DEBUGS_LOGE("LED GPIO init failed: %s", esp_err_to_name(err));
    return err;
  }

  BaseType_t ret =
      xTaskCreate(led_task, "led_indicator_task", LED_TASK_STACK_SIZE, NULL,
                  LED_TASK_PRIORITY, &led_task_handle);

  if (ret != pdPASS) {
    DEBUGS_LOGE("Failed to create LED task");
    return ESP_FAIL;
  }

  led_initialized = true;
  DEBUGS_LOGI("LED initialized on GPIO %d, active_%s", led_gpio,
              led_active_high ? "HIGH" : "LOW");
#endif
  return ESP_OK;
}

esp_err_t led_indicator_set_mode(led_mode_t mode) {
  if (!led_initialized)
    return ESP_ERR_INVALID_STATE;

  if (mode < LED_MODE_OFF || mode > LED_MODE_BLINK_ERROR)
    return ESP_ERR_INVALID_ARG;

  portENTER_CRITICAL(&led_mux);
  current_mode = mode;
  portEXIT_CRITICAL(&led_mux);

  return ESP_OK;
}

void led_indicator_deinit(void) {
  if (!led_initialized)
    return;

  vTaskDelete(led_task_handle);
  led_set(false);
  led_task_handle = NULL;
  led_initialized = false;
  DEBUGS_LOGI("LED deinitialized");
}
