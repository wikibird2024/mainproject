#ifndef SIM4G_GPS_H
#define SIM4G_GPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "data_manager_types.h" // NEW: Include this to use the gps_data_t struct
#include "esp_err.h"

/**
 * @file sim4g_gps.h
 * @brief High-level GPS + SMS + MQTT public interface for EC800K/4G module.
 */

/**
 * @brief Initialize SIM4G GPS subsystem.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sim4g_gps_init(void);

/**
 * @brief Set target phone number for alert SMS.
 *
 * @param number Null-terminated string (e.g., "+849xxxxxxxx")
 * @return ESP_OK on success.
 */
void sim4g_gps_set_phone_number(const char *number);

/**
 * @brief Check GPS power status.
 *
 * @param[out] enabled Output flag (true = on, false = off)
 * @return ESP_OK if command successful, ESP_FAIL otherwise
 */
esp_err_t sim4g_gps_is_enabled(bool *enabled);

/**
 * @brief Starts a task to handle a fall alert.
 *
 * This function creates a new, non-blocking task that will send both an SMS
 * and an MQTT message.
 *
 * @param gps_data A pointer to the latest GPS data at the time of the fall.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t sim4g_gps_start_fall_alert(const gps_data_t *gps_data);

/**
 * @brief Updates the GPS location by communicating with the SIM4G module.
 */
void sim4g_gps_update_location(void);

#ifdef __cplusplus
}
#endif

#endif // SIM4G_GPS_H
