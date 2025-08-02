#include "wifi_connect.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <string.h>

#define TAG "WIFI_CONNECT"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_MAX_RETRY CONFIG_WIFI_MAX_RETRY

static EventGroupHandle_t wifi_event_group;
static int retry_count = 0;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id,
                               void *data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    if (retry_count < WIFI_MAX_RETRY) {
      esp_wifi_connect();
      retry_count++;
      ESP_LOGW(TAG, "Retry %d/%d", retry_count, WIFI_MAX_RETRY);
    } else {
      ESP_LOGE(TAG, "Max retries reached");
    }
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
    ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    retry_count = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

esp_err_t wifi_connect_sta(TickType_t timeout_ticks) {
  // 1. Khởi tạo NFS (giữ nguyên)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  // 2. Thiết lập network stack (giữ nguyên)
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  // 3. Cấu hình WiFi (giữ nguyên)
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // 4. Đăng ký event handler (giữ nguyên)
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));

  // 5. Thiết lập thông số WiFi (giữ nguyên)
  wifi_config_t wifi_config = {0};
  strlcpy((char *)wifi_config.sta.ssid, CONFIG_WIFI_SSID,
          sizeof(wifi_config.sta.ssid));
  strlcpy((char *)wifi_config.sta.password, CONFIG_WIFI_PASSWORD,
          sizeof(wifi_config.sta.password));
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  // 6. Khởi động WiFi (giữ nguyên)
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  // 7. Xử lý timeout (chỉ thêm tham số timeout_ticks)
  wifi_event_group = xEventGroupCreate();
  EventBits_t bits = xEventGroupWaitBits(
      wifi_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdFALSE,
      timeout_ticks // Sử dụng tham số truyền vào thay vì config
  );

  // 8. Kiểm tra kết nối (giữ nguyên)
  if (bits & WIFI_CONNECTED_BIT) {
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "Connection timeout after %" PRIu32 "ms",
             pdTICKS_TO_MS(timeout_ticks));
    return ESP_FAIL;
  }
}
