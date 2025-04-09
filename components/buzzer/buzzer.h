#pragma once

#include "esp_err.h"

/**
 * @brief Khởi tạo buzzer (GPIO, task xử lý bật/tắt)
 * 
 * @return ESP_OK nếu thành công, ESP_FAIL nếu lỗi
 */
esp_err_t buzzer_init(void);

/**
 * @brief Bật còi trong duration_ms mili giây (non-blocking)
 * 
 * @param duration_ms Thời gian còi kêu (ms)
 * @return ESP_OK nếu gửi thành công, ESP_FAIL nếu queue đầy
 */
esp_err_t buzzer_beep(int duration_ms);
