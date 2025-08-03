#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System status structure
 */
typedef struct {
  bool system_initialized;
  bool application_running;
  bool mutex_available;
  bool event_queue_available;
  bool event_handler_initialized;
  bool wifi_initialized; // Track wifi status
  bool wifi_connected;
} app_system_status_t;

// ─────────────────────────────────────────────────────────────────────────────
// Main Application Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize the entire system
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t app_system_init(void);

/**
 * @brief Start the application tasks
 * @return ESP_OK on success, ESP_FAIL on failure, ESP_ERR_INVALID_STATE if
 * system not initialized
 */
esp_err_t app_start_application(void);

/**
 * @brief Stop the application
 * @return ESP_OK on success
 */
esp_err_t app_stop_application(void);

/**
 * @brief Restart the entire system
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t app_restart_system(void);

// ─────────────────────────────────────────────────────────────────────────────
// Status and Information Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Check if system is initialized
 * @return true if initialized, false otherwise
 */
bool app_is_system_initialized(void);

/**
 * @brief Check if application is running
 * @return true if running, false otherwise
 */
bool app_is_application_running(void);

/**
 * @brief Get system status information
 * @param status Pointer to status structure to fill
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if status is NULL
 */
esp_err_t app_get_system_status(app_system_status_t *status);

// ─────────────────────────────────────────────────────────────────────────────
// Legacy Getter Functions (for backward compatibility)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Get mutex handle (legacy function)
 * @deprecated Use proper initialization flow instead
 * @return Mutex handle or NULL if not initialized
 */
SemaphoreHandle_t get_mutex(void);

/**
 * @brief Get event queue handle (legacy function)
 * @deprecated Use proper initialization flow instead
 * @return Queue handle or NULL if not initialized
 */
QueueHandle_t get_event_queue(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H_
