#pragma once

#include "driver/gpio.h" // Thư viện điều khiển GPIO
#include "esp_err.h"      // Thư viện quản lý lỗi

// Định nghĩa chân GPIO cho buzzer
#define BUZZER_GPIO GPIO_NUM_25  // Thay số 25 bằng chân thực tế bạn sử dụng

/**
 * @brief Khởi tạo buzzer (Cấu hình GPIO output)
 */
void buzzer_init(void);

/**
 * @brief Bật còi
 */
void buzzer_on(void);

/**
 * @brief Tắt còi
 */
void buzzer_off(void);

