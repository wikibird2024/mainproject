/**
 * @file json_wrapper.h
 * @brief Public API for the JSON serialization module.
 *
 * This module is responsible for creating JSON payloads from device data. It
 * simplifies the process of formatting data for transmission over network
 * protocols like MQTT or HTTP.
 *
 * @author Hao Tran
 * @date 2025
 */
#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "data_manager_types.h"

/**
 * @brief Creates a JSON payload representing the current device status.
 *
 * This function retrieves data from the data manager and formats it into a
 * JSON string containing information like Wi-Fi status, MQTT connection,
 * and GPS data.
 *
 * @warning The returned string must be freed by the caller to prevent memory
 * leaks.
 *
 * @return A pointer to a dynamically allocated JSON string.
 */
char *json_wrapper_create_status_payload(void);

/**
 * @brief Creates a JSON payload for a fall alert event.
 *
 * This function generates a specific JSON string to signal that a fall has
 * been detected, including relevant information such as the device ID and
 * timestamp.
 *
 * @warning The returned string must be freed by the caller to prevent memory
 * leaks.
 *
 * @return A pointer to a dynamically allocated JSON string.
 */
char *json_wrapper_create_alert_payload(void);

#ifdef __cplusplus
}
#endif

#endif // JSON_WRAPPER_H
