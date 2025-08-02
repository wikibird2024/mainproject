
/**
 * @file debugs.c
 * @brief Debug and logging module with optional periodic system log task.
 *
 * Features controlled via menuconfig:
 * - CONFIG_DEBUGS_ENABLE_LOG
 * - CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
 * - CONFIG_DEBUGS_LOG_INTERVAL_MS
 * - CONFIG_DEBUGS_TASK_STACK_SIZE
 * - CONFIG_DEBUGS_TASK_PRIORITY
 */

#include "debugs.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h> // <== Đã thêm để dùng PRIu32

#define INFO(...) DEBUGS_LOGI(__VA_ARGS__)
#define ERROR(...) DEBUGS_LOGE(__VA_ARGS__)

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG

static TaskHandle_t s_debugs_task_handle = NULL;
static volatile bool s_debugs_task_running = false;

/**
 * @brief Periodic logging task for system monitoring.
 */
static void debugs_periodic_task(void *arg) {
  DEBUGS_LOGI("Periodic log task started.");
  s_debugs_task_running = true;

  while (s_debugs_task_running) {
    INFO("System running normally. Free heap: %" PRIu32 " bytes",
         esp_get_free_heap_size()); // <== Đã sửa %d -> %" PRIu32 "
    vTaskDelay(pdMS_TO_TICKS(CONFIG_DEBUGS_LOG_INTERVAL_MS));
  }

  INFO("Periodic logging task exited.");
  s_debugs_task_handle = NULL; // Clear handle after clean exit
  vTaskDelete(NULL);
}
#endif // CONFIG_DEBUGS_ENABLE_PERIODIC_LOG

void debugs_init(void) {
#if CONFIG_DEBUGS_ENABLE_LOG
  esp_log_level_set(DEBUGS_TAG, ESP_LOG_INFO);
  INFO("Debug system initialized.");
#endif

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
  debugs_set_periodic_log(true);
#endif
}

void debugs_set_periodic_log(bool enable) {
#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
  if (enable && s_debugs_task_handle == NULL) {
    BaseType_t result = xTaskCreate(
        debugs_periodic_task, "debugs_task", CONFIG_DEBUGS_TASK_STACK_SIZE,
        NULL, CONFIG_DEBUGS_TASK_PRIORITY, &s_debugs_task_handle);

    if (result == pdPASS) {
      INFO("Periodic logging enabled.");
    } else {
      ERROR("Failed to create periodic logging task.");
      s_debugs_task_handle = NULL;
    }

  } else if (!enable && s_debugs_task_handle != NULL) {
    s_debugs_task_running = false;
    INFO("Disabling periodic logging...");
    // Let the task exit and nullify its handle
  }
#else
  (void)enable; // Avoid unused parameter warning
#endif
}
