/**
 * @file sim4g_at.h
 * @brief Public interface for the low-level SIM4G AT command driver.
 *
 * This file provides function prototypes for interacting with the 4G-GPS module
 * via AT commands. It abstracts the underlying UART communication and command
 * management.
 */

#pragma once

#include <string.h>
#include "esp_err.h"
#include "sim4g_at_cmd.h" // This header file contains at_cmd_id_t and at_command_t definitions

#ifdef __cplusplus
extern "C" {
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Function Prototypes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initializes the SIM4G AT command driver.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t sim4g_at_init(void);

/**
 * @brief Checks if the module is registered on the cellular network.
 * @return ESP_OK if registered, otherwise ESP_FAIL.
 */
esp_err_t sim4g_at_check_network_registration(void);

/**
 * @brief Sends an AT command specified by its ID and waits for a response.
 * @param cmd_id The ID of the AT command to send (from at_cmd_id_t enum).
 * @param response A buffer to store the modem's response. Can be NULL.
 * @param len The size of the response buffer.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t sim4g_at_send_by_id(at_cmd_id_t cmd_id, char *response, size_t len);

/**
 * @brief Configures GPS settings before enabling it.
 * @return ESP_OK if successful, otherwise an error code.
 */
esp_err_t sim4g_at_configure_gps(void);

/**
 * @brief Enables the GPS functionality on the module.
 * @return ESP_OK if GPS is successfully enabled, ESP_FAIL otherwise.
 */
esp_err_t sim4g_at_enable_gps(void);

/**
 * @brief Retrieves the current GPS location data from the module.
 * @param timestamp Buffer to store the timestamp string.
 * @param lat Buffer to store the latitude string.
 * @param lon Buffer to store the longitude string.
 * @return ESP_OK if location data is successfully retrieved and parsed,
 * ESP_ERR_INVALID_ARG for invalid pointers, or ESP_FAIL on error.
 */
esp_err_t sim4g_at_get_location(char *timestamp, char *lat, char *lon);

/**
 * @brief Sends an SMS message to a specified phone number.
 * @param phone The phone number to send the SMS to (e.g., "+84123456789").
 * @param message The content of the SMS message.
 * @return ESP_OK if the SMS is successfully sent, ESP_FAIL otherwise.
 */
esp_err_t sim4g_at_send_sms(const char *phone, const char *message);

#ifdef __cplusplus
}
#endif
