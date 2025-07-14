/**
 * @file led_indicator.h
 * @brief Interface for controlling a single LED indicator with predefined blink patterns.
 *
 * This component allows switching LED modes (blinking, steady on/off) for alerts/status.
 * It runs a FreeRTOS task internally to handle blink timing.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief LED Indicator modes.
 */
typedef enum {
    LED_MODE_OFF = 0,        ///< LED off
    LED_MODE_ON,             ///< LED steady on
    LED_MODE_BLINK_FAST,     ///< Fast blink (e.g., alert)
    LED_MODE_BLINK_SLOW,     ///< Slow heartbeat blink
    LED_MODE_BLINK_ERROR     ///< Error pattern (double blink)
} led_mode_t;

/**
 * @brief Initialize the LED indicator system.
 *
 * This starts the internal LED control task.
 *
 * @return ESP_OK on success.
 */
esp_err_t led_indicator_init(void);

/**
 * @brief Set the LED mode.
 *
 * @param mode One of the predefined led_mode_t values.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if invalid.
 */
esp_err_t led_indicator_set_mode(led_mode_t mode);

/**
 * @brief Deinitialize the LED indicator task and turn LED off.
 *
 * Optional cleanup at shutdown.
 */
void led_indicator_deinit(void);

#ifdef __cplusplus
}
#endif
