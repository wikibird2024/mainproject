#ifndef DEBUGS_H
#define DEBUGS_H

#include "esp_log.h"

// Định nghĩa macro log với mức độ khác nhau
#define DEBUG(fmt, ...) ESP_LOGD("DEBUGS", fmt, ##__VA_ARGS__)
#define INFO(fmt, ...)  ESP_LOGI("DEBUGS", fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)  ESP_LOGW("DEBUGS", fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) ESP_LOGE("DEBUGS", fmt, ##__VA_ARGS__)

// Hàm khởi tạo log
void debugs_init(void);

#endif // DEBUGS_H

