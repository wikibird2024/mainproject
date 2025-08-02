#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sim4g_gps.h
 * @brief Public API for SIM4G GPS location and SMS alert services.
 *
 * This module provides high-level functions to control GPS functionality
 * and send SMS alerts using a 4G SIM module (e.g., EC800K).
 *
 * It encapsulates lower-level AT command communication and exposes
 * simplified interfaces for location retrieval and emergency SMS sending.
 */

/**
 * @brief Struct to hold parsed GPS location data.
 */
typedef struct {
  bool valid;         /**< Indicates whether the GPS data is valid */
  char timestamp[20]; /**< UTC timestamp, format: "YYYYMMDDHHMMSS" */
  char latitude[20];  /**< Latitude in decimal degrees */
  char longitude[20]; /**< Longitude in decimal degrees */
} sim4g_gps_data_t;

/**
 * @brief Initialize the SIM4G GPS module.
 *
 * Enables GPS, performs required configuration.
 */
void sim4g_gps_init(void);

/**
 * @brief Set the phone number for SMS alerts.
 *
 * @param number Phone number in international format (e.g., "+84123456789")
 */
void sim4g_gps_set_phone_number(const char *number);

/**
 * @brief Retrieve the latest GPS location.
 *
 * This function returns the latest valid GPS data, if available.
 *
 * @return sim4g_gps_data_t Struct containing timestamp, latitude, and longitude
 */
sim4g_gps_data_t sim4g_gps_get_location(void);

/**
 * @brief Send a fall alert SMS with GPS coordinates.
 *
 * @param location Pointer to GPS data to include in the SMS
 */
void send_fall_alert_sms(const sim4g_gps_data_t *location);

/**
 * @brief Check whether GPS functionality is currently enabled.
 *
 * @return true if GPS is enabled, false otherwise
 */
bool sim4g_gps_is_enabled(void);

void sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *data,
                                     void (*callback)(bool success));

/**
 * @brief Update the current GPS location and store it in the provided struct.
 *
 * @param[out] out Pointer to struct where GPS data will be stored
 * @return true if valid GPS data was obtained, false otherwise
 */
bool sim4g_gps_update_location(sim4g_gps_data_t *out);

#ifdef __cplusplus
}
#endif
