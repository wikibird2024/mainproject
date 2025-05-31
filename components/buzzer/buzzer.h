/**
 * @file buzzer.h
 * @brief Driver điều khiển buzzer GPIO (Active Buzzer) cho ESP32
 * @author Your Name/Team Name
 * @date 31/05/2025
 * @version 2.0
 * @copyright Copyright (c) 2025 Your Company. All rights reserved.
 * @note Chỉ hỗ trợ buzzer chủ động (active buzzer). Đối với buzzer bị động, dùng driver PWM riêng.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BUZZER_DRIVER Driver Buzzer
 * @brief Nhóm hàm API điều khiển buzzer
 * @{
 */

/**
 * @brief Khởi tạo buzzer (GPIO, task xử lý, queue)
 * @return 
 *      - ESP_OK: Thành công
 *      - ESP_FAIL: Lỗi khởi tạo GPIO/queue
 * @warning Chỉ gọi hàm này một lần duy nhất trong quá trình khởi động hệ thống.
 */
esp_err_t buzzer_init(void);

/**
 * @brief Bật còi trong thời gian xác định (non-blocking)
 * @param duration_ms Thời gian bật (ms). 
 *        - >0: Bật trong khoảng thời gian (ví dụ: 1000 = 1 giây)
 *        - 0: Tắt ngay lập tức
 *        - <0: Bật vô hạn (phải gọi lại với duration_ms=0 để tắt)
 * @return 
 *      - ESP_OK: Gửi lệnh thành công
 *      - ESP_FAIL: Queue chưa được khởi tạo hoặc đầy
 * @example 
 * @code
 * buzzer_beep(1000);  // Beep 1 giây
 * buzzer_beep(-1);    // Bật vô hạn
 * buzzer_beep(0);     // Tắt
 * @endcode
 */
esp_err_t buzzer_beep(int duration_ms);

/** @} */ // End of BUZZER_DRIVER

#ifdef __cplusplus
}
#endif