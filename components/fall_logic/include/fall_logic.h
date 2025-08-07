#ifndef _FALL_LOGIC_H_
#define _FALL_LOGIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "sdkconfig.h"
#include "stdbool.h" // Thêm include để sử dụng kiểu bool

#if CONFIG_FALL_LOGIC_ENABLE

/**
 * @brief Initializes the fall detection module.
 *
 * This function sets up internal resources needed by the module.
 * The module is self-contained and does not require external FreeRTOS handles.
 *
 * @return
 * - ESP_OK on success
 * - ESP_FAIL if an internal error occurs.
 */
esp_err_t fall_logic_init(void);

/**
 * @brief Starts the fall detection task.
 *
 * This launches a FreeRTOS task that reads sensor data and performs
 * fall detection continuously.
 *
 * @return
 * - ESP_OK on success
 * - ESP_FAIL if task creation fails.
 */
esp_err_t fall_logic_start(void);

/**
 * @brief Enables fall detection logic at runtime.
 * @return ESP_OK on success.
 */
esp_err_t fall_logic_enable(void);

/**
 * @brief Disables fall detection logic at runtime.
 * @return ESP_OK on success.
 */
esp_err_t fall_logic_disable(void);

/**
 * @brief Checks if fall detection logic is currently active.
 * @return true if enabled, false otherwise.
 */
bool fall_logic_is_enabled(void);

/**
 * @brief Resets the fall detection status, allowing for new fall events to be
 * triggered.
 *
 * This function should be called by the event handler after an alert has been
 * completed.
 *
 * @return ESP_OK on success.
 */
esp_err_t fall_logic_reset_fall_status(void);

#else // CONFIG_FALL_LOGIC_ENABLE disabled

// Fallback macros if fall logic is not enabled in Kconfig
#define fall_logic_init() (ESP_OK)
#define fall_logic_start() (ESP_OK)
#define fall_logic_enable() (ESP_OK)
#define fall_logic_disable() (ESP_OK)
#define fall_logic_is_enabled() (false)
#define fall_logic_reset_fall_status() (ESP_OK) // Fallback cho hàm mới

#endif // CONFIG_FALL_LOGIC_ENABLE

#ifdef __cplusplus
}
#endif

#endif // _FALL_LOGIC_H_
