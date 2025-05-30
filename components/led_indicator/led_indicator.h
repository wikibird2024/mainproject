#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file led_indicator.h
 * @brief Giao diện điều khiển LED đa chế độ có hỗ trợ đa luồng, sử dụng trong hệ thống nhúng ESP32
 *
 * Module này cung cấp các hàm điều khiển LED (bật, tắt, nháy) một cách an toàn trong môi trường FreeRTOS.
 * Được thiết kế với tính mô-đun cao, thread-safe, phù hợp dùng trong các sản phẩm nhúng chuyên nghiệp.
 */

/**
 * @brief Các chế độ nháy LED được hỗ trợ
 */
typedef enum {
    LED_BLINK_SINGLE = 0,   ///< Nháy 1 lần mỗi chu kỳ
    LED_BLINK_DOUBLE,       ///< Nháy 2 lần mỗi chu kỳ
    LED_BLINK_TRIPLE        ///< Nháy 3 lần mỗi chu kỳ
} led_blink_mode_t;

/**
 * @brief Cấu hình khởi tạo cho LED indicator
 */
typedef struct {
    int gpio_num;           ///< Số chân GPIO điều khiển LED
    bool active_high;       ///< true: mức HIGH bật LED; false: mức LOW bật LED
} led_indicator_config_t;

/**
 * @brief Khởi tạo hệ thống điều khiển LED
 *
 * Hàm này cần được gọi một lần trước khi sử dụng các hàm điều khiển khác.
 *
 * @param config Cấu hình chân GPIO và mức kích hoạt
 * @return ESP_OK nếu thành công, mã lỗi khác nếu thất bại
 */
esp_err_t led_indicator_init(const led_indicator_config_t *config);

/**
 * @brief Bắt đầu nháy LED theo chu kỳ và chế độ định trước
 *
 * Nếu đang có task nháy LED đang chạy, nó sẽ được dừng lại và khởi động lại với thông số mới.
 *
 * @param delay_ms Độ dài chu kỳ nháy (ms), trong khoảng [10, 10000]
 * @param mode Chế độ nháy: đơn, đôi hoặc ba
 * @return ESP_OK nếu thành công, mã lỗi nếu thất bại (ESP_ERR_INVALID_ARG, ESP_ERR_INVALID_STATE,...)
 */
esp_err_t led_indicator_start_blink(uint32_t delay_ms, led_blink_mode_t mode);

/**
 * @brief Dừng nháy LED và tắt LED
 */
void led_indicator_stop_blink(void);

/**
 * @brief Bật LED thủ công, dừng bất kỳ task nháy nào nếu đang chạy
 */
void led_indicator_on(void);

/**
 * @brief Tắt LED thủ công, dừng bất kỳ task nháy nào nếu đang chạy
 */
void led_indicator_off(void);

#ifdef __cplusplus
}
#endif
