
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Device data structure for MQTT publishing
 */
typedef struct {
  char device_id[32]; /*!< Unique ID of the device */
  bool fall_detected; /*!< True if fall is detected */
  char timestamp[20]; /*!< Timestamp of the event */
  double latitude;    /*!< Latitude GPS coordinate */
  double longitude;   /*!< Longitude GPS coordinate */
} device_data_t;

/**
 * @brief Initialize and start MQTT client (as publisher only)
 */
void mqtt_client_start(void);

/**
 * @brief Publish device data in JSON format to default topic
 *
 * @param[in] data Pointer to the device data structure
 */
void mqtt_client_publish_data(const device_data_t *data);

/**
 * @brief Publish raw JSON string to a given topic
 *
 * @param[in] topic MQTT topic to publish to
 * @param[in] json_str JSON string to publish
 */
void mqtt_client_publish_json(const char *topic, const char *json_str);

/**
 * @brief Build JSON string from device data
 *
 * @param[in] data Pointer to device data
 * @return char* Allocated JSON string (must be freed by caller)
 */
char *mqtt_client_build_json(const device_data_t *data);

#ifdef __cplusplus
}
#endif
