#include "debugs.h"

void debugs_init(void)
{
    esp_log_level_set("*", ESP_LOG_DEBUG);  // Bật tất cả log ở mức DEBUG
    INFO("Debugs system initialized.");
}

