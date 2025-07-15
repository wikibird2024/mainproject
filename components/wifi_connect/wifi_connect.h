
/**
 * @file wifi_connect.h
 * @brief Wi-Fi station connection interface using ESP-IDF with logging
 * abstraction.
 *
 * This module provides an easy-to-use interface for connecting an ESP32 device
 * to a Wi-Fi network in station mode. It uses the ESP-IDF Wi-Fi stack and
 * integrates structured logging through the `debugs` component.
 */

#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connect to a Wi-Fi network in station (STA) mode.
 *
 * Initializes Wi-Fi, registers event handlers, and attempts to connect to the
 * specified SSID. The function blocks until either:
 * - The device connects successfully and receives an IP (returns `ESP_OK`), or
 * - The `timeout_ticks` is reached (returns `ESP_FAIL`).
 *
 * @param ssid          SSID of the Wi-Fi network.
 * @param password      Password of the Wi-Fi network.
 * @param timeout_ticks Timeout for waiting connection, in FreeRTOS ticks.
 *                      Pass `portMAX_DELAY` to wait indefinitely.
 * @return `ESP_OK` on successful connection, `ESP_FAIL` on timeout,
 *         or `ESP_ERR_INVALID_ARG` if parameters are invalid.
 */
esp_err_t wifi_connect_sta(const char *ssid, const char *password,
                           TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif
