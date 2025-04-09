#include "debugs.h"

void debugs_init(void)
{
    // Có thể cấu hình từng TAG hoặc toàn hệ thống
    esp_log_level_set(DEBUG_TAG, ESP_LOG_DEBUG);

    // Nếu muốn bật toàn bộ log hệ thống trong giai đoạn debug:
    // esp_log_level_set("*", ESP_LOG_DEBUG); 

    INFO("Debugs system initialized.");
}
