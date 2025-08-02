wifi component config in kconfi and this is how to use in main.c


'''c
#include "wifi_connect.h"
#include "esp_log.h"

void app_main(void) {
    if (wifi_connect_sta(pdMS_TO_TICKS(10000)) == ESP_OK) {
        ESP_LOGI("APP", "Wi-Fi connected");
    } else {
        ESP_LOGE("APP", "Wi-Fi failed to connect");
    }
}
'''
