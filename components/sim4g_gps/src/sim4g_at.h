
/**
 * @file sim4g_at.h
 * @brief Public interface for the low-level SIM4G AT command driver.
 *
 * This file provides function prototypes for interacting with the 4G-GPS module
 * via AT commands. It abstracts the underlying UART communication and command
 * management.
 */

#pragma once

#include <stddef.h>
#include "esp_err.h"
#include "sim4g_at_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sends an AT command specified by its ID and waits for a response.
 *
 * This is the core function for sending AT commands. It uses a predefined
 * command table to get the command string and its associated timeout.
 *
 * @param cmd_id The ID of the AT command to send (from at_cmd_id_t enum).
 * @param response A buffer to store the modem's response. Can be NULL if no response is expected.
 * @param len The size of the response buffer.
 * @return esp_err_t ESP_OK on success, or an error code on failure (e.g., ESP_ERR_TIMEOUT, ESP_FAIL).
 */
esp_err_t sim4g_at_send_by_id(at_cmd_id_t cmd_id, char *response, size_t len);

/**
 * @brief Enables the GPS functionality on the module.
 *
 * This function sends the necessary AT commands to turn on the GNSS receiver.
 * It also optionally enables the `autogps` feature.
 *
 * @return esp_err_t ESP_OK if GPS is successfully enabled, ESP_FAIL otherwise.
 */
esp_err_t sim4g_at_enable_gps(void);

/**
 * @brief Retrieves the current GPS location data from the module.
 *
 * This function sends the AT+QGPSLOC command and parses the response to
 * extract the timestamp, latitude, and longitude.
 *
 * @param timestamp Buffer to store the timestamp string.
 * @param lat Buffer to store the latitude string.
 * @param lon Buffer to store the longitude string.
 * @param len The maximum size of each buffer (timestamp, lat, and lon).
 * @return esp_err_t ESP_OK if location data is successfully retrieved and parsed,
 *         ESP_ERR_INVALID_ARG for invalid pointers, or ESP_FAIL on error.
 */
esp_err_t sim4g_at_get_location(char *timestamp, char *lat, char *lon, size_t len);

/**
 * @brief Sends an SMS message to a specified phone number.
 *
 * This function handles the multi-step process of sending an SMS using AT commands,
 * including setting the mode and sending the message body with Ctrl+Z termination.
 *
 * @param phone The phone number to send the SMS to (e.g., "+84123456789").
 * @param message The content of the SMS message.
 * @return esp_err_t ESP_OK if the SMS is successfully sent, ESP_FAIL otherwise.
 */
esp_err_t sim4g_at_send_sms(const char *phone, const char *message);

#ifdef __cplusplus
}
#endif
