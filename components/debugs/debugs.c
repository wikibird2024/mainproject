#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define DEBUG_TAG "DEBUGS"

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
static TaskHandle_t s_debugs_task_handle = NULL;

static void debugs_periodic_task(void *arg)
{
    while (1) {
        INFO("System running normally...");
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DEBUGS_LOG_INTERVAL_MS));
    }
}
#endif

void debugs_init(void)
{
#if CONFIG_DEBUGS_ENABLE_LOG
    esp_log_level_set("*", ESP_LOG_INFO); // Có thể thay "*" bằng DEBUG_TAG nếu chỉ muốn log riêng component này
    INFO("Debugs system initialized.");
#endif

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
    if (s_debugs_task_handle == NULL) {
        xTaskCreate(debugs_periodic_task, "debugs_task", 2048, NULL, 5, &s_debugs_task_handle);
    }
#endif
}

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
    (void)enable; // tránh warning nếu cấu hình bị tắt
#endif
}
