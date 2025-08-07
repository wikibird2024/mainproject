#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "data_manager_types.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initializes the data management module.
 *
 * This function creates the data mutex and initializes the device's state.
 * @return esp_err_t ESP_OK on successful initialization.
 */
esp_err_t data_manager_init(void);

/**
 * @brief Deinitializes the data management module.
 */
void data_manager_deinit(void);

/**
 * @brief Gets a copy of the current device state.
 *
 * This function is thread-safe and returns a copy of the state data.
 * @param[out] state A pointer to the device_state_t structure to store the
 * copy.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t data_manager_get_device_state(device_state_t *state);

/**
 * @brief Sets the entire device state.
 *
 * This function is thread-safe and allows updating all fields of the state at
 * once.
 * @param state A pointer to the device_state_t structure with the new data.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t data_manager_set_device_state(const device_state_t *state);

/**
 * @brief Gets the current fall detection status.
 */
bool data_manager_get_fall_status(void);

/**
 * @brief Gets the current GPS data.
 *
 * This function is thread-safe.
 * @param[out] data A pointer to the gps_data_t structure to store the
 * data.
 */
esp_err_t data_manager_get_gps_data(gps_data_t *data);

/**
 * @brief Gets the current WiFi connection status.
 */
bool data_manager_get_wifi_status(void);

/**
 * @brief Gets the current MQTT connection status.
 */
bool data_manager_get_mqtt_status(void);

/**
 * @brief Get the device ID.
 * @param id_buffer Buffer to copy the device ID into.
 * @param buffer_size Size of the buffer.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if buffer is NULL or size is
 * 0.
 */
esp_err_t data_manager_get_device_id(char *id_buffer, size_t buffer_size);

/**
 * @brief Sets the fall detection status.
 * @param status The new fall status.
 */
esp_err_t data_manager_set_fall_status(bool status);

/**
 * @brief Sets the GPS data.
 *
 * This function is thread-safe.
 * @param data A pointer to the gps_data_t structure containing the new data.
 */
esp_err_t data_manager_set_gps_data(const gps_data_t *data);

/**
 * @brief Sets the WiFi connection status.
 * @param connected The new connection status.
 */
esp_err_t data_manager_set_wifi_status(bool connected);

/**
 * @brief Sets the MQTT connection status.
 * @param connected The new connection status.
 */
esp_err_t data_manager_set_mqtt_status(bool connected);

/**
 * @brief Sets the SIM registration status.
 * @param registered The new registration status.
 */
esp_err_t data_manager_set_sim_status(bool registered);

/**
 * @brief Sets the device ID.
 * @param id A pointer to the device ID string.
 */
esp_err_t data_manager_set_device_id(const char *id);

#ifdef __cplusplus
}
#endif

#endif // _DATA_MANAGER_H_manager.h
