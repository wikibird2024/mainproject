/**
 * @file app_main.h
 * @brief Main application control and public API.
 *
 * This header defines the core functions for managing the entire system's
 * lifecycle, including initialization, starting, and stopping. It serves as
 * the main entry point for the application's top-level logic.
 *
 * @author Hao Tran
 * @date 2025
 */
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Main Application Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initializes the entire system.
 *
 * This function handles the setup for all modules, including hardware and
 * software components.
 *
 * @return esp_err_t ESP_OK on success, ESP_FAIL on a critical initialization
 * failure.
 */
esp_err_t app_system_init(void);

/**
 * @brief Starts the main application tasks.
 *
 * This function launches all the primary tasks that perform the application's
 * core functionality (e.g., fall detection, communication).
 *
 * @return
 * - ESP_OK: All tasks started successfully.
 * - ESP_FAIL: A task failed to start.
 * - ESP_ERR_INVALID_STATE: The system has not been initialized.
 */
esp_err_t app_start_application(void);

/**
 * @brief Stops the main application tasks.
 *
 * This function gracefully stops and cleans up all running application tasks.
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_stop_application(void);

/**
 * @brief Restarts the entire system.
 *
 * This function gracefully stops, deinitializes, and then reinitializes the
 * system.
 *
 * @return esp_err_t ESP_OK on a successful restart, ESP_FAIL on failure.
 */
esp_err_t app_restart_system(void);

// ─────────────────────────────────────────────────────────────────────────────
// Status and Information Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Checks if the system has been initialized.
 *
 * @return true if the system is initialized, otherwise false.
 */
bool app_is_system_initialized(void);

/**
 * @brief Checks if the application tasks are currently running.
 *
 * @return true if the application is running, otherwise false.
 */
bool app_is_application_running(void);

/**
 * @brief Checks if the Wi-Fi is connected.
 *
 * This is a convenience function that queries the Wi-Fi connection module.
 *
 * @return true if connected, false otherwise.
 */
bool app_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H_
