
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Initialize the buzzer (PWM setup via comm).
 */
esp_err_t buzzer_init(void);

/**
 * @brief Beep the buzzer for the given duration (in ms).
 *
 * @param duration_ms Duration to beep.
 * @return esp_err_t ESP_OK if successful.
 */
esp_err_t buzzer_beep(uint32_t duration_ms);

/**
 * @brief Stop the buzzer immediately.
 */
esp_err_t buzzer_stop(void);

#ifdef __cplusplus
}
#endif
