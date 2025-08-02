
/**
 * @file led_indicator.h
 * @brief LED Indicator API
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  LED_MODE_OFF = 0,
  LED_MODE_ON,
  LED_MODE_BLINK_FAST,
  LED_MODE_BLINK_SLOW,
  LED_MODE_BLINK_ERROR
} led_mode_t;

/**
 * @brief Initialize LED indicator module
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t led_indicator_init(void);

/**
 * @brief Set LED operating mode
 */
esp_err_t led_indicator_set_mode(led_mode_t mode);

/**
 * @brief Get current LED mode
 */
led_mode_t led_indicator_get_mode(void);

/**
 * @brief Deinitialize LED and stop task
 */
void led_indicator_deinit(void);

#ifdef __cplusplus
}
#endif
