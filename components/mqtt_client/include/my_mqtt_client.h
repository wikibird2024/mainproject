
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "common/device_data.h"  // Shared definition of device_data_t

/**
 * @brief Initialize and start the MQTT client as a publisher.
 *
 * This sets up the MQTT client using parameters from Kconfig.
 * Must be called once during system startup.
 */
void mqtt_client_start(void);

/**
 * @brief Publish device data as JSON to the default topic.
 *
 * @param[in] data Pointer to the structured device data.
 */
void mqtt_client_publish_data(const device_data_t *data);

/**
 * @brief Publish a raw JSON string to a specific MQTT topic.
 *
 * @param[in] topic     MQTT topic to publish to.
 * @param[in] json_str  Null-terminated JSON string.
 */
void mqtt_client_publish_json(const char *topic, const char *json_str);

/**
 * @brief Build a JSON string from structured device data.
 *
 * @param[in]  data Pointer to the device data.
 * @return     Allocated JSON string (caller must `free()` it).
 */
char *mqtt_client_build_json(const device_data_t *data);

#ifdef __cplusplus
}
#endif
