/**
 * @file debugs.c
 * @brief Module xử lý log và debug định kỳ cho hệ thống.
 *
 * Cung cấp khả năng ghi log theo thời gian thực, kiểm tra trạng thái hệ thống
 * và hỗ trợ tắt/bật log định kỳ thông qua FreeRTOS task.
 *
 * Cấu hình có thể bật/tắt qua menuconfig:
 * - CONFIG_DEBUGS_ENABLE_LOG
 * - CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
 * - CONFIG_DEBUGS_LOG_INTERVAL_MS
 */
#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Tag mặc định cho log; có thể thay đổi nếu muốn log riêng cho từng component
#define DEBUG_TAG "DEBUGS"

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
static TaskHandle_t s_debugs_task_handle = NULL;  // Handle của task log định kỳ

// Task log định kỳ – ghi log trạng thái hệ thống theo chu kỳ cố định
static void debugs_periodic_task(void *arg)
{
    while (1) {
        INFO("System running normally...");
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DEBUGS_LOG_INTERVAL_MS));
    }
}
#endif  // CONFIG_DEBUGS_ENABLE_PERIODIC_LOG

/**
 * @brief Khởi tạo hệ thống debug
 *
 * Thiết lập mức độ log và khởi chạy task log định kỳ nếu được cấu hình.
 */
void debugs_init(void)
{
#if CONFIG_DEBUGS_ENABLE_LOG
    // Thiết lập mức log mặc định cho tất cả các tag (hoặc DEBUG_TAG nếu muốn giới hạn)
    esp_log_level_set("*", ESP_LOG_INFO);  // Có thể thay "*" bằng DEBUG_TAG nếu chỉ muốn log riêng component này
    INFO("Debugs system initialized.");
#endif

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
    if (s_debugs_task_handle == NULL) {
        xTaskCreate(debugs_periodic_task, "debugs_task", 2048, NULL, 5, &s_debugs_task_handle);
    }
#endif
}

/**
 * @brief Bật/Tắt log định kỳ
 *
 * @param enable true để bật log định kỳ, false để tắt
 */
void debugs_set_periodic_log(bool enable)
{
#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
    if (enable && s_debugs_task_handle == NULL) {
        xTaskCreate(debugs_periodic_task, "debugs_task", 2048, NULL, 5, &s_debugs_task_handle);
        INFO("Periodic log enabled.");
    } else if (!enable && s_debugs_task_handle != NULL) {
        vTaskDelete(s_debugs_task_handle);
        s_debugs_task_handle = NULL;
        INFO("Periodic log disabled.");
    }
#else
    (void)enable;  // Tránh cảnh báo 'unused parameter' nếu tính năng bị tắt
#endif
}
