#ifndef EVENT_HANDLER_H_
#define EVENT_HANDLER_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the event handler with provided queue
 * @param queue Queue handle for receiving fall events
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if queue is NULL, ESP_FAIL if
 * task creation fails
 */
esp_err_t event_handler_init(QueueHandle_t queue);

/**
 * @brief Deinitialize the event handler
 * @return ESP_OK on success
 */
esp_err_t event_handler_deinit(void);

/**
 * @brief Check if event handler is initialized
 * @return true if initialized, false otherwise
 */
bool event_handler_is_initialized(void);

/**
 * @brief Get the queue handle (for backward compatibility or debugging)
 * @return Queue handle or NULL if not initialized
 */
QueueHandle_t event_handler_get_queue(void);

#ifdef __cplusplus
}
#endif

#endif // EVENT_HANDLER_H_
