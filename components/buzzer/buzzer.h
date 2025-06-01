/**
 * @file buzzer.h
 * @brief Giao diện driver điều khiển buzzer active trên ESP32
 */

#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo buzzer
 * @return ESP_OK nếu thành công, ESP_FAIL nếu lỗi
 */
esp_err_t buzzer_init(void);

/**
 * @brief Điều khiển buzzer bật/tắt theo thời gian
 * @param duration_ms Thời gian bật (ms). >0: bật trong thời gian xác định;
 *                    0: tắt ngay; <0: bật vô hạn
 * @return ESP_OK nếu OK, ESP_FAIL nếu lỗi
 */
esp_err_t buzzer_beep(int duration_ms);

/**
 * @brief Tắt buzzer ngay lập tức
 * @return ESP_OK nếu OK, ESP_FAIL nếu lỗi
 */
esp_err_t buzzer_stop(void);

#ifdef __cplusplus
}
#endif