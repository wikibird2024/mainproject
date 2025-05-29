#pragma once

#include <stdint.h>
#include "esp_err.h"

// Khởi tạo chân GPIO LED (phải gọi trước khi dùng)
esp_err_t led_indicator_init(int gpio_num);

// Bắt đầu task nháy LED với khoảng thời gian delay_ms giữa bật và tắt
esp_err_t led_indicator_start_blink(uint32_t delay_ms);

// Dừng task nháy LED
void led_indicator_stop_blink(void);

// Bật LED (thủ công, nếu muốn)
void led_indicator_on(void);

// Tắt LED (thủ công, nếu muốn)
void led_indicator_off(void);

