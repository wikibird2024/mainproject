#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Enum of system events to be handled.
 *
 * This provides a simple, structured way to pass different event types
 * through the event queue.
 */
typedef enum {
    EVENT_NONE = 0,
    EVENT_FALL_DETECTED,
    EVENT_WIFI_CONNECTED,
    EVENT_MQTT_CONNECTED,
    EVENT_MAX
} system_event_t;

/**
 * @brief Initializes the event handler module.
 *
 * This function creates an internal FreeRTOS task and a queue to listen for
 * system events. It does not require any external handles.
 *
 * @return
 * - ESP_OK on success.
 * - ESP_FAIL if task or queue creation fails.
 */
esp_err_t event_handler_init(void);

/**
 * @brief Deinitializes the event handler module.
 *
 * This function stops the event handling task and frees resources.
 *
 * @return ESP_OK on success.
 */
esp_err_t event_handler_deinit(void);

/**
 * @brief Sends a system event to the event handler's internal queue.
 *
 * This is the primary function for other modules (e.g., data_manager) to
 * notify the event handler of significant changes or occurrences.
 *
 * @param event The event to be sent.
 * @return
 * - ESP_OK on success.
 * - ESP_FAIL if the queue is full or not initialized.
 */
esp_err_t event_handler_send_event(system_event_t event);

#ifdef __cplusplus
}
#endif

#endif // _EVENT_HANDLER_H_
