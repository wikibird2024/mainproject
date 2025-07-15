#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float latitude;
  float longitude;
  float altitude;
  int utc_hour;
  int utc_min;
  int utc_sec;
  bool valid;
} gps_location_t;

/**
 * @brief Initialize SIM module (check UART, send AT).
 */
esp_err_t sim4g_init(void);

/**
 * @brief Enable GPS (cold start).
 */
esp_err_t sim4g_enable_gps(void);

/**
 * @brief Get GPS location.
 */
esp_err_t sim4g_get_location(gps_location_t *loc);

/**
 * @brief Send SMS with content to a phone number.
 */
esp_err_t sim4g_send_sms(const char *phone_number, const char *message);

#ifdef __cplusplus
}
#endif

