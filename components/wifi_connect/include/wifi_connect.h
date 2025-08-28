/**
 * @file wifi_connect.h
 * @brief Public API for the Wi-Fi connection module.
 *
 * This module manages all Wi-Fi connectivity, including connecting to an access
 * point, managing disconnections, and retrieving connection status. It
 * abstracts the low-level ESP-IDF Wi-Fi driver complexity.
 *
 * @author Hao Tran
 * @date 2025
 */
#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// Connection Management
/**
 * @brief Connects the ESP32 in Station mode to a configured Wi-Fi network.
 *
 * This function attempts to connect to the Wi-Fi network using the credentials
 * set by wifi_set_credentials().
 *
 * @param timeout_ticks The maximum time to wait for a connection, in FreeRTOS
 * ticks.
 * @return
 * - ESP_OK: Connection successful.
 * - ESP_ERR_TIMEOUT: The connection timed out.
 * - Other ESP-IDF error codes on failure.
 */
esp_err_t wifi_connect_sta(TickType_t timeout_ticks);

/**
 * @brief Disconnects the ESP32 from the current Wi-Fi network.
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t wifi_disconnect(void);

/**
 * @brief Reconnects the ESP32 to the previously connected Wi-Fi network.
 *
 * @param timeout_ticks The maximum time to wait for reconnection, in FreeRTOS
 * ticks.
 * @return
 * - ESP_OK: Reconnection successful.
 * - ESP_ERR_TIMEOUT: The reconnection timed out.
 * - Other ESP-IDF error codes on failure.
 */
esp_err_t wifi_reconnect(TickType_t timeout_ticks);

// Status & Info
/**
 * @brief Checks if the Wi-Fi is currently connected.
 *
 * @return true if Wi-Fi is connected, false otherwise.
 */
bool wifi_is_connected(void);

/**
 * @brief Gets the device's assigned IP address.
 *
 * @param ip_str A pointer to the character array to store the IP address
 * string.
 * @param ip_str_len The size of the provided character array.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t wifi_get_ip_info(char *ip_str, size_t ip_str_len);

/**
 * @brief Gets the current RSSI (Received Signal Strength Indicator).
 *
 * @return The RSSI value in dBm.
 */
int wifi_get_rssi(void);

// Configuration
/**
 * @brief Sets the Wi-Fi credentials (SSID and password).
 *
 * These credentials are used for connecting in Station mode.
 *
 * @param ssid The Wi-Fi network SSID.
 * @param password The Wi-Fi network password.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t wifi_set_credentials(const char *ssid, const char *password);

/**
 * @brief Starts the ESP32 in Access Point (AP) mode.
 *
 * This function allows the ESP32 to act as a Wi-Fi hotspot.
 *
 * @param ssid The SSID for the new access point.
 * @param password The password for the new access point.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t wifi_start_ap_mode(const char *ssid, const char *password);

// Cleanup
/**
 * @brief De-initializes the Wi-Fi connection module.
 *
 * This function frees up all resources used by the module.
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t wifi_connect_deinit(void);

#endif // WIFI_CONNECT_H
