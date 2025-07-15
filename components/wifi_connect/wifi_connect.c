
/**
 * @file wifi_connect.c
 * @brief Wi-Fi station mode connection module using ESP-IDF with debugs
 * integration.
 */

#include "wifi_connect.h"
#include "debugs.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include <string.h>

#define WIFI_MAX_RETRY 5
#define WIFI_CONNECTED_BIT BIT0

/// Logging tag for this module
#define TAG "WIFI"

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();

  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_count < WIFI_MAX_RETRY) {
      esp_wifi_connect();
      s_retry_count++;
      WARN("[%s] Retry to connect to AP... (%d/%d)", TAG, s_retry_count,
           WIFI_MAX_RETRY);
    } else {
      ERROR("[%s] Connection to AP failed after maximum retries", TAG);
    }

  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    INFO("[%s] Got IP: " IPSTR, TAG, IP2STR(&event->ip_info.ip));
    s_retry_count = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

esp_err_t wifi_connect_sta(const char *ssid, const char *password,
                           TickType_t timeout_ticks) {
  if (!ssid || strlen(ssid) == 0) {
    ERROR("[%s] Invalid SSID", TAG);
    return ESP_ERR_INVALID_ARG;
  }

  // Init network stack & Wi-Fi
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));

  // Set Wi-Fi config
  wifi_config_t wifi_config = {0};
  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
  strncpy((char *)wifi_config.sta.password, password,
          sizeof(wifi_config.sta.password) - 1);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  // Apply and start
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  INFO("[%s] Wi-Fi STA started, SSID: %s", TAG, ssid);

  // Wait for IP
  s_wifi_event_group = xEventGroupCreate();
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdTRUE, pdFALSE, timeout_ticks);

  if (bits & WIFI_CONNECTED_BIT) {
    INFO("[%s] Wi-Fi connected successfully", TAG);
    return ESP_OK;
  } else {
    ERROR("[%s] Wi-Fi connection timeout", TAG);
    return ESP_FAIL;
  }
}
