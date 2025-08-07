/**
 * @file led_indicator.c
 * @brief Implementation of LED Indicator control with various blinking modes.
 */

#include "led_indicator.h"
#include "driver/gpio.h" // New: Use ESP-IDF GPIO driver directly
#include "esp_log.h"
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

static const char *TAG = "LED_INDICATOR";

static TaskHandle_t led_task_handle = NULL;
static led_mode_t current_mode = LED_MODE_OFF;

static const int led_gpio = CONFIG_LED_INDICATOR_GPIO;
static const bool led_active_high = CONFIG_LED_INDICATOR_ACTIVE_HIGH;

static portMUX_TYPE led_mux = portMUX_INITIALIZER_UNLOCKED;

// New: Macro to simplify critical sections
#define LED_ENTER_CRITICAL() portENTER_CRITICAL(&led_mux)
#define LED_EXIT_CRITICAL() portEXIT_CRITICAL(&led_mux)

/**
 * @brief Internal: set the LED level using ESP-IDF GPIO driver.
 *
 * This function handles the active-high/active-low logic internally.
 *
 * @param on True to turn the LED on, false to turn it off.
 */
static inline void led_set(bool on) {
  gpio_set_level(led_gpio, led_active_high ? on : !on);
}

/**
 * @brief Internal: main LED control task.
 *
 * This task controls the LED's state based on the current mode.
 * It remains responsive by yielding or delaying as needed.
 *
 * @param arg Task parameter (unused).
 */
static void led_task(void *arg) {
  while (1) {
    led_mode_t mode;

    LED_ENTER_CRITICAL();
    mode = current_mode;
    LED_EXIT_CRITICAL();

    switch (mode) {
    case LED_MODE_OFF:
      led_set(false);
      // No need for a continuous delay, just yield to other tasks.
      vTaskDelay(1);
      break;

    case LED_MODE_ON:
      led_set(true);
      // No need for a continuous delay, just yield to other tasks.
      vTaskDelay(1);
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
  // New: Check if task handle already exists instead of a separate flag
  if (led_task_handle != NULL) {
    ESP_LOGW(TAG, "LED already initialized.");
    return ESP_OK;
  }

  // New: Use gpio_config_t to set up the GPIO pin
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << led_gpio),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  esp_err_t err = gpio_config(&io_conf);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "LED GPIO init failed: %s", esp_err_to_name(err));
    return err;
  }

  // New: Ensure LED starts in the OFF state
  led_set(false);

  BaseType_t ret =
      xTaskCreate(led_task, "led_indicator_task", LED_TASK_STACK_SIZE, NULL,
                  LED_TASK_PRIORITY, &led_task_handle);

  if (ret != pdPASS) {
    ESP_LOGE(TAG, "Failed to create LED task");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "LED initialized on GPIO %d, active_%s", led_gpio,
           led_active_high ? "HIGH" : "LOW");
#endif
  return ESP_OK;
}

esp_err_t led_indicator_set_mode(led_mode_t mode) {
  // New: Check if task handle is NULL instead of a separate flag
  if (led_task_handle == NULL)
    return ESP_ERR_INVALID_STATE;

  if (mode < LED_MODE_OFF || mode > LED_MODE_BLINK_ERROR)
    return ESP_ERR_INVALID_ARG;

  LED_ENTER_CRITICAL();
  current_mode = mode;
  LED_EXIT_CRITICAL();

  return ESP_OK;
}

void led_indicator_deinit(void) {
  if (led_task_handle == NULL)
    return;

  // New: Ensure the GPIO is cleaned up
  gpio_reset_pin(led_gpio);
  vTaskDelete(led_task_handle);
  led_task_handle = NULL;
  ESP_LOGI(TAG, "LED deinitialized");
}
