#pragma once

#include "esp_log.h"

/**
 * @brief Định nghĩa TAG mặc định dùng cho toàn bộ log của component này
 *
 * TAG này sẽ xuất hiện trong tất cả các log để dễ dàng phân biệt trên terminal.
 */
#define DEBUG_TAG "DEBUGS"

/**
 * @brief Ghi log ở mức DEBUG (dành cho thông tin chi tiết phục vụ debug)
 */
#define DEBUG(fmt, ...) ESP_LOGD(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Ghi log ở mức INFO (dành cho thông tin tổng quan hệ thống)
 */
#define INFO(fmt, ...)  ESP_LOGI(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Ghi log ở mức WARN (cảnh báo, nhưng chưa gây lỗi hệ thống)
 */
#define WARN(fmt, ...)  ESP_LOGW(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Ghi log ở mức ERROR (lỗi nghiêm trọng ảnh hưởng chức năng)
 */
#define ERROR(fmt, ...) ESP_LOGE(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Khởi tạo hệ thống log
 *
 * Hàm này thiết lập mức log mặc định toàn hệ thống là `ESP_LOG_DEBUG`
 * để hiển thị đầy đủ thông tin khi phát triển và debug.
 *
 * Nên gọi hàm này một lần trong `app_main()` hoặc khởi tạo hệ thống.
 */
void debugs_init(void);
