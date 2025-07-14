/**
 * @file debugs.h
 * @brief Logging and debug interface with optional periodic logging task.
 *
 * This module provides macro-based logging abstraction and runtime control
 * over a periodic FreeRTOS debug task, enabled via Kconfig.
 *
 * Controlled via menuconfig:
 * - CONFIG_DEBUGS_ENABLE_LOG             Enable/disable all logging macros.
 * - CONFIG_DEBUGS_ENABLE_PERIODIC_LOG   Enable/disable periodic system log task.
 * - CONFIG_DEBUGS_LOG_INTERVAL_MS       Log task interval in milliseconds.
 * - CONFIG_DEBUGS_TASK_STACK_SIZE       Stack size of the debug task.
 * - CONFIG_DEBUGS_TASK_PRIORITY         Priority of the debug task.
 */

#pragma once

#include <stdbool.h>
#include "esp_log.h"

/// Default logging tag for the debug system
#define DEBUGS_TAG "DEBUGS"

#ifdef CONFIG_DEBUGS_ENABLE_LOG
    #define DEBUGS_LOGD(fmt, ...) ESP_LOGD(DEBUGS_TAG, fmt, ##__VA_ARGS__)
    #define DEBUGS_LOGI(fmt, ...) ESP_LOGI(DEBUGS_TAG, fmt, ##__VA_ARGS__)
    #define DEBUGS_LOGW(fmt, ...) ESP_LOGW(DEBUGS_TAG, fmt, ##__VA_ARGS__)
    #define DEBUGS_LOGE(fmt, ...) ESP_LOGE(DEBUGS_TAG, fmt, ##__VA_ARGS__)
#else
    #define DEBUGS_LOGD(fmt, ...)
    #define DEBUGS_LOGI(fmt, ...)
    #define DEBUGS_LOGW(fmt, ...)
    #define DEBUGS_LOGE(fmt, ...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the debug logging system.
 *
 * Should be called once during application startup (e.g., in `app_main()`).
 */
void debugs_init(void);

/**
 * @brief Enable or disable periodic logging at runtime.
 *
 * @param enable true to start periodic log, false to stop and release resources.
 */
void debugs_set_periodic_log(bool enable);

#ifdef __cplusplus
}
#endif
