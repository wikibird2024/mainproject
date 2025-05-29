/**
 * @file debugs.h
 * @brief Interface for debugs component – quản lý logging hệ thống và log định kỳ
 */

#pragma once
#include <stdbool.h>
#include "esp_log.h"

// Tag mặc định dùng cho các log của component debugs, có thể thay đổi nếu cần log riêng
#define DEBUG_TAG "DEBUGS" // Tag mặc định cho log, có thể thay đổi để phân biệt module


// Macro ghi log theo các mức độ khác nhau
#ifdef CONFIG_DEBUGS_ENABLE_LOG
 /**
 * @brief Macro log mức DEBUG
 */
    #define DEBUG(fmt, ...) ESP_LOGD(DEBUG_TAG, fmt, ##__VA_ARGS__)
/**
 * @brief Macro log mức INFO
 */
    #define INFO(fmt, ...)  ESP_LOGI(DEBUG_TAG, fmt, ##__VA_ARGS__)
/**
 * @brief Macro log mức WARN (cảnh báo)
 */
    #define WARN(fmt, ...)  ESP_LOGW(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Macro log mức ERROR (báo lỗi)
 */
    #define ERROR(fmt, ...) ESP_LOGE(DEBUG_TAG, fmt, ##__VA_ARGS__)
#else
    #define DEBUG(fmt, ...)
    #define INFO(fmt, ...)
    #define WARN(fmt, ...)
    #define ERROR(fmt, ...)
#endif

/**
 * @brief Thiết lập mức log mặc định là INFO.
 *
 * Nên gọi một lần trong app_main() để bật log chi tiết 
 */
void debugs_init(void);
/**
 * @brief Bật hoặc tắt log định kỳ trong runtime.
 * 
 * Nếu cấu hình `CONFIG_DEBUGS_ENABLE_PERIODIC_LOG` được bật trong menuconfig, 
 * hàm này sẽ cho phép log theo chu kỳ với khoảng thời gian được định nghĩa trong `CONFIG_DEBUGS_LOG_INTERVAL_MS`.
 * 
 * @param enable Trạng thái bật/tắt log định kỳ. `true` để bật, `false` để tắt.
 */
void debugs_set_periodic_log(bool enable);

