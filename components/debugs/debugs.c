
#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define DEBUG_TAG "DEBUGS"

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
static void debugs_periodic_task(void *arg)
{
    while (1) {
        ESP_LOGI(DEBUG_TAG, "System running normally...");
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DEBUGS_LOG_INTERVAL_MS));
    }
}
#endif

void debugs_init(void)
{
    esp_log_level_set(DEBUG_TAG, ESP_LOG_DEBUG); 
    ESP_LOGI(DEBUG_TAG, "Debugs system initialized.");

#if CONFIG_DEBUGS_ENABLE_PERIODIC_LOG
    xTaskCreate(debugs_periodic_task, "debugs_task", 2048, NULL, 5, NULL);
#endif
}
