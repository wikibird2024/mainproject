#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// Connection Management
esp_err_t wifi_connect_sta(TickType_t timeout_ticks);
esp_err_t wifi_disconnect(void);
esp_err_t wifi_reconnect(TickType_t timeout_ticks);

// Status & Info
bool wifi_is_connected(void);
esp_err_t wifi_get_ip_info(char *ip_str, size_t ip_str_len);
int wifi_get_rssi(void);

// Configuration
esp_err_t wifi_set_credentials(const char *ssid, const char *password);
esp_err_t wifi_start_ap_mode(const char *ssid, const char *password);

// Cleanup
esp_err_t wifi_connect_deinit(void);

#endif // WIFI_CONNECT_H
