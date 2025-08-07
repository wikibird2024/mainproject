#ifndef USER_MQTT_H
#define USER_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "mqtt_client.h" // Needed for esp_mqtt_client_handle_t

/**
 * @file user_mqtt.h
 * @brief High-level MQTT client public interface.
 *
 * Encapsulates the initialization and event handling for the MQTT client.
 */

/**
 * @brief Initializes and connects the MQTT client.
 *
 * @param broker_uri The URI of the MQTT broker (e.g.,
 * "mqtt://broker.hivemq.com").
 * @return esp_err_t ESP_OK on success, an error code otherwise.
 */
esp_err_t user_mqtt_init(const char *broker_uri);

/**
 * @brief Returns the global MQTT client handle.
 *
 * This function is used by other components (like sim4g_gps) to get the
 * client handle for publishing messages.
 *
 * @return esp_mqtt_client_handle_t The MQTT client handle.
 */
esp_mqtt_client_handle_t user_mqtt_get_client(void);

#ifdef __cplusplus
}
#endif

#endif // USER_MQTT_H
