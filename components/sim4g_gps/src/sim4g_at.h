/**
 * @file sim4g_at.h
 * @brief Public interface for the low-level SIM4G AT command driver.
 *
 * This file provides function prototypes for interacting with the 4G-GPS module
 * via AT commands. It abstracts the underlying UART communication and command
 * management.
 */

#pragma once

#include "data_manager_types.h" // NEW: For the gps_data_t struct
#include "esp_err.h"
#include "sim4g_at_cmd.h"
#include <string.h> // Recommended for clarity

#ifdef __cplusplus
extern "C" {
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Function Prototypes
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t sim4g_at_init(void);
esp_err_t sim4g_at_configure_apn(const char *apn);
esp_err_t sim4g_at_check_network_registration(void);
esp_err_t sim4g_at_send_by_id(at_cmd_id_t cmd_id, char *response, size_t len);
esp_err_t sim4g_at_configure_gps(void);
esp_err_t sim4g_at_enable_gps(void);

// This is the function you were missing. It retrieves GPS data into a struct.
esp_err_t sim4g_at_get_gps(gps_data_t *gps_data);

// This is the old function. It's likely not needed anymore.
// We are keeping it here but the new function above is better.
esp_err_t sim4g_at_get_location(char *timestamp, char *lat, char *lon);

esp_err_t sim4g_at_send_sms(const char *phone, const char *message);

#ifdef __cplusplus
}
#endif
