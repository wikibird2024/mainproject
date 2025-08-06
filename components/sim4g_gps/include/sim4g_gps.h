#pragma once

#include "esp_err.h"
#include <stdbool.h> // Thêm stdbool.h để định nghĩa kiểu bool

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sim4g_gps.h
 * @brief High-level GPS + SMS public interface for EC800K/4G module.
 *
 * Encapsulates location retrieval and SMS alerts using AT commands.
 */

/**
 * @brief Parsed GPS data (decimal degrees).
 */
typedef struct {
  bool valid;         /**< Whether GPS data is valid */
  char timestamp[32]; /**< UTC time: "YYYYMMDDHHMMSS" */
  double latitude;    /**< Latitude in decimal degrees */
  double longitude;   /**< Longitude in decimal degrees */
  bool has_gps_fix; /**< Flag to indicate if GPS is valid */
} sim4g_gps_data_t;

esp_err_t sim4g_gps_update_location(void);
/**
 * @brief Initialize SIM4G GPS subsystem.
 *
 * Creates internal resources and powers on GNSS module.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sim4g_gps_init(void);

/**
 * @brief Set target phone number for alert SMS.
 *
 * @param number Null-terminated string (e.g., "+849xxxxxxxx")
 * @return ESP_OK if number is valid and stored, ESP_ERR_INVALID_ARG otherwise
 */
esp_err_t sim4g_gps_set_phone_number(const char *number);

/**
 * @brief Check GPS power status.
 *
 * @param[out] enabled Output flag (true = on, false = off)
 * @return ESP_OK if command successful, ESP_FAIL otherwise
 */
esp_err_t sim4g_gps_is_enabled(bool *enabled);

/**
 * @brief Get current GPS location and update it in the data_manager.
 *
 * @note This function handles communication with the GPS module.
 *
 * @return ESP_OK if valid data obtained and updated, ESP_FAIL otherwise
 */
esp_err_t sim4g_gps_update_location(void);

/**
 * @brief Send SMS alert with GPS info (blocking).
 *
 * @param[in] location Pointer to GPS data to embed in message
 * @return ESP_OK if sent, ESP_FAIL otherwise
 */
esp_err_t sim4g_gps_send_fall_alert_sms(const sim4g_gps_data_t *location);

/**
 * @brief Asynchronous version of fall alert (non-blocking).
 *
 * @param[in] location Valid GPS info
 * @param[in] callback Optional callback (pass NULL if unused)
 */
esp_err_t sim4g_gps_send_fall_alert_async(sim4g_gps_data_t data);

#ifdef __cplusplus
}
#endif
