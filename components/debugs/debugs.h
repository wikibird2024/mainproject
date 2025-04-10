#pragma once
#include <stdbool.h>
#include "esp_log.h"

// TAG mặc định cho log của component
#define DEBUG_TAG "DEBUGS"

// Macro ghi log theo các mức độ khác nhau
#define DEBUG(fmt, ...) ESP_LOGD(DEBUG_TAG, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)  ESP_LOGI(DEBUG_TAG, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)  ESP_LOGW(DEBUG_TAG, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) ESP_LOGE(DEBUG_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Thiết lập mức log mặc định là DEBUG.
 *
 * Nên gọi một lần trong app_main() để bật log chi tiết 
 */
void debugs_init(void);
/**
 * @brief Bật hoặc tắt log định kỳ trong runtime.
 * Có tác dụng nếu Kconfig DEBUGS_ENABLE_PERIODIC_LOG được bật.
 */
void debugs_set_periodic_log(bool enable);

