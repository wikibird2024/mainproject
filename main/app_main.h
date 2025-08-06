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
 * @brief Initialize the entire system.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t app_system_init(void);

/**
 * @brief Start the application tasks.
 * @return ESP_OK on success, ESP_FAIL on failure, ESP_ERR_INVALID_STATE if
 * system not initialized.
 */
esp_err_t app_start_application(void);

/**
 * @brief Stop the application.
 * @return ESP_OK on success.
 */
esp_err_t app_stop_application(void);

/**
 * @brief Restart the entire system.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t app_restart_system(void);

// ─────────────────────────────────────────────────────────────────────────────
// Status and Information Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Check if system is initialized.
 * @return true if initialized, false otherwise.
 */
bool app_is_system_initialized(void);

/**
 * @brief Check if application is running.
 * @return true if running, false otherwise.
 */
bool app_is_application_running(void);

/**
 * @brief Check if WiFi is connected.
 * @return true if connected, false otherwise.
 */
bool app_is_wifi_connected(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H_
