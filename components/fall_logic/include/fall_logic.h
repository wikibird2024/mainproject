/**
 * @file fall_logic.h
 * @brief API Interface for the Fall Detection Module.
 *
 * This module provides public functions to initialize, control, and monitor
 * the fall detection logic. It is designed as a standalone component for
 * easy integration into the system.
 *
 * @author Hao Tran
 * @date 2025
 */
#ifndef _FALL_LOGIC_H_
#define _FALL_LOGIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "sdkconfig.h"
#include "stdbool.h"

#if CONFIG_FALL_LOGIC_ENABLE

/**
 * @brief Initializes the fall detection module.
 *
 * This function sets up all necessary internal resources for the module.
 *
 * @return
 * - ESP_OK: Initialization was successful.
 * - ESP_FAIL: An internal error occurred.
 */
esp_err_t fall_logic_init(void);

/**
 * @brief Starts the fall detection task.
 *
 * This function launches a dedicated FreeRTOS task to continuously read sensor
 * data and perform fall logic analysis.
 *
 * @return
 * - ESP_OK: The task was created successfully.
 * - ESP_FAIL: The task creation failed.
 */
esp_err_t fall_logic_start(void);

/**
 * @brief Enables the fall detection logic.
 *
 * Allows the detection logic to operate during runtime.
 * @return ESP_OK on success.
 */
esp_err_t fall_logic_enable(void);

/**
 * @brief Disables the fall detection logic.
 *
 * Pauses the data analysis and fall detection process.
 * @return ESP_OK on success.
 */
esp_err_t fall_logic_disable(void);

/**
 * @brief Checks if the fall detection logic is currently active.
 *
 * @return true if the fall detection logic is enabled, otherwise false.
 */
bool fall_logic_is_enabled(void);

/**
 * @brief Resets the fall detection status.
 *
 * This function should be called after a fall event has been processed and an
 * alert has been sent, allowing the module to be ready for new fall events.
 *
 * @return ESP_OK on success.
 */
esp_err_t fall_logic_reset_fall_status(void);

#else // CONFIG_FALL_LOGIC_ENABLE disabled

// Fallback macros when the module is disabled in Kconfig.
#define fall_logic_init() (ESP_OK)
#define fall_logic_start() (ESP_OK)
#define fall_logic_enable() (ESP_OK)
#define fall_logic_disable() (ESP_OK)
#define fall_logic_is_enabled() (false)
#define fall_logic_reset_fall_status() (ESP_OK)

#endif // CONFIG_FALL_LOGIC_ENABLE

#ifdef __cplusplus
}
#endif

#endif // _FALL_LOGIC_H_
