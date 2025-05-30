/**
 * @file debugs.h
 * @brief Giao diện cho module debugs – quản lý logging hệ thống và ghi log định kỳ.
 *
 * Cung cấp các macro tiện ích để ghi log (DEBUG, INFO, WARN, ERROR) và hàm để khởi tạo, bật/tắt log định kỳ.
 *
 * Cấu hình bật/tắt tính năng này được điều khiển qua menuconfig:
 * - CONFIG_DEBUGS_ENABLE_LOG: Bật/tắt toàn bộ hệ thống log.
 * - CONFIG_DEBUGS_ENABLE_PERIODIC_LOG: Bật task ghi log trạng thái theo chu kỳ.
 * - CONFIG_DEBUGS_LOG_INTERVAL_MS: Khoảng thời gian log định kỳ (miligiây).
 */

#pragma once

#include <stdbool.h>
#include "esp_log.h"

/// @brief Tag mặc định cho log của component debugs
#define DEBUG_TAG "DEBUGS"

#ifdef CONFIG_DEBUGS_ENABLE_LOG

/**
 * @brief Macro log mức DEBUG
 * @note Chỉ hoạt động nếu CONFIG_DEBUGS_ENABLE_LOG được bật trong menuconfig
 */
#define DEBUG(fmt, ...) ESP_LOGD(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Macro log mức INFO
 * @note Chỉ hoạt động nếu CONFIG_DEBUGS_ENABLE_LOG được bật trong menuconfig
 */
#define INFO(fmt, ...)  ESP_LOGI(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Macro log mức WARN (cảnh báo)
 * @note Chỉ hoạt động nếu CONFIG_DEBUGS_ENABLE_LOG được bật trong menuconfig
 */
#define WARN(fmt, ...)  ESP_LOGW(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Macro log mức ERROR (báo lỗi)
 * @note Chỉ hoạt động nếu CONFIG_DEBUGS_ENABLE_LOG được bật trong menuconfig
 */
#define ERROR(fmt, ...) ESP_LOGE(DEBUG_TAG, fmt, ##__VA_ARGS__)

#else

#define DEBUG(fmt, ...)
#define INFO(fmt, ...)
#define WARN(fmt, ...)
#define ERROR(fmt, ...)

#endif  // CONFIG_DEBUGS_ENABLE_LOG

/**
 * @brief Khởi tạo hệ thống log.
 *
 * Thiết lập mức log mặc định là INFO cho toàn bộ hệ thống (hoặc chỉ riêng component nếu sửa đổi tag).
 *
 * @note Nên gọi một lần trong hàm app_main(). Nếu không gọi, log có thể không hiển thị đúng mức hoặc không hiển thị gì cả.
 */
void debugs_init(void);

/**
 * @brief Bật hoặc tắt log định kỳ trong runtime.
 *
 * Nếu cấu hình `CONFIG_DEBUGS_ENABLE_PERIODIC_LOG` được bật trong menuconfig,
 * hàm này sẽ cho phép log trạng thái hệ thống theo chu kỳ với khoảng thời gian được định nghĩa trong `CONFIG_DEBUGS_LOG_INTERVAL_MS`.
 *
 * @param enable Trạng thái bật/tắt log định kỳ. `true` để bật, `false` để tắt.
 *
 * @note Khi bật, task định kỳ sẽ được tạo và ghi log liên tục. Khi tắt, task sẽ bị xóa để giải phóng tài nguyên.
 */
void debugs_set_periodic_log(bool enable);
