#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi động Wi-Fi Station và kết nối vào SSID được config
 *
 * @param timeout_ticks Thời gian chờ kết nối (ticks)
 * @return esp_err_t ESP_OK nếu thành công, lỗi nếu thất bại
 */
esp_err_t wifi_connect_sta(TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif
