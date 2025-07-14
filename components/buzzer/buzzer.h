/**
 * @file buzzer.h
 * @brief Buzzer control interface using PWM.
 *
 * This component provides initialization and control functions
 * for an active buzzer via the PWM abstraction in the comm module.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Initialize the buzzer system.
 *
 * This must be called once before buzzer_on() or buzzer_off().
 *
 * @return ESP_OK on success, or an error code from comm_pwm_init().
 */
esp_err_t buzzer_init(void);

/**
 * @brief Turn the buzzer ON.
 *
 * Starts PWM signal to activate the buzzer.
 *
 * @return ESP_OK on success, or ESP_FAIL if buzzer is not initialized.
 */
esp_err_t buzzer_on(void);

/**
 * @brief Turn the buzzer OFF.
 *
 * Stops the PWM signal.
 *
 * @return ESP_OK on success, or ESP_FAIL if buzzer is not initialized.
 */
esp_err_t buzzer_off(void);

#ifdef __cplusplus
}
#endif
