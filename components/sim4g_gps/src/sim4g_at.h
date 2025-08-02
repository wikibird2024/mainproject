#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sim4g_at.h
 * @brief AT Command interface for SIM7600/EC800K modules.
 *
 * This module provides low-level functions to send AT commands
 * and parse specific responses (GPS, SMS, etc.).
 *
 * Used internally by sim4g_gps.c and other upper-layer modules.
 */

/**
 * @brief Send a generic AT command and get the response.
 *
 * @param[in] cmd       Null-terminated AT command (e.g. "AT+CSQ").
 * @param[out] response Buffer to store response string.
 * @param[in] len       Maximum length of the response buffer.
 * @param[in] delay_ms  Delay after sending command, before reading response.
 *
 * @return true if response received successfully, false otherwise.
 */
bool sim4g_at_send_cmd(const char *cmd, char *response, size_t len,
                       int delay_ms);

/**
 * @brief Enable GNSS/GPS on the module.
 *
 * This typically sends "AT+CGNSPWR=1" or equivalent command.
 *
 * @return true if GPS power-on command succeeded, false otherwise.
 */
bool sim4g_at_enable_gps(void);

/**
 * @brief Get current GPS location from module.
 *
 * Internally sends AT command and parses the response for time, latitude, and
 * longitude.
 *
 * @param[out] timestamp  Buffer to store UTC timestamp string.
 * @param[out] lat        Buffer to store latitude.
 * @param[out] lon        Buffer to store longitude.
 * @param[in] len         Maximum length for each buffer (timestamp, lat, lon).
 *
 * @return true if valid location retrieved, false otherwise.
 */
bool sim4g_at_get_location(char *timestamp, char *lat, char *lon, size_t len);

/**
 * @brief Send SMS using AT command.
 *
 * @param[in] phone   Null-terminated string with phone number (e.g.,
 * "+849xxxxxxxx").
 * @param[in] message Message content to send (null-terminated).
 *
 * @return true if SMS sent successfully, false otherwise.
 */
bool sim4g_at_send_sms(const char *phone, const char *message);

#ifdef __cplusplus
}
#endif
