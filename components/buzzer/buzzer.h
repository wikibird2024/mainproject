
/**
 * @file buzzer.h
 * @brief Buzzer control interface for ESP32 (Active/Passive Buzzer).
 *
 * This driver supports both active and passive buzzers.
 * - Active Buzzer: controlled via GPIO (on/off)
 * - Passive Buzzer: controlled via PWM signal
 *
 * The behavior is configurable via buzzer_config.h.
 * Internally, this driver runs a FreeRTOS task and uses a queue
 * to manage buzzer commands asynchronously.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Initialize the buzzer system.
 *
 * This sets up the GPIO or PWM (depending on configuration),
 * creates a command queue and starts the internal buzzer task.
 *
 * @return ESP_OK on success, or error code otherwise.
 */
esp_err_t buzzer_init(void);

/**
 * @brief Trigger the buzzer for a specific duration.
 *
 * If duration_ms > 0: buzzer will turn ON for that number of milliseconds then
 * turn OFF. If duration_ms == 0: buzzer will turn OFF immediately. If
 * duration_ms < 0: buzzer will stay ON until buzzer_stop() is called.
 *
 * @param duration_ms Duration in milliseconds (or <0 for ON indefinitely).
 * @return ESP_OK on success, ESP_FAIL if not initialized or queue full.
 */
esp_err_t buzzer_beep(int duration_ms);

/**
 * @brief Immediately stop the buzzer.
 *
 * Equivalent to buzzer_beep(0).
 *
 * @return ESP_OK on success, ESP_FAIL if not initialized.
 */
esp_err_t buzzer_stop(void);

#ifdef __cplusplus
}
#endif
