/**
 * @file wifi_connect.c
 * @brief Enterprise-grade WiFi connection management module
 * @version 1.0.0
 * @date 2025-08-03
 *
 * This module provides comprehensive WiFi connectivity management including
 * STA mode connection, AP mode hosting, status monitoring, and automatic
 * reconnection capabilities with enterprise-level error handling and logging.
 */

#include "wifi_connect.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
// Module Configuration and Constants
// ─────────────────────────────────────────────────────────────────────────────

#define TAG "WIFI_CONNECT"

// Event bits for WiFi state management
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_DISCONNECTED_BIT BIT2

// Default values and limits
#define WIFI_DEFAULT_TIMEOUT_MS CONFIG_WIFI_CONNECT_TIMEOUT_MS
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASSWORD_MAX_LEN 64
#define WIFI_IP_STR_MAX_LEN 16

// State management
typedef enum {
  WIFI_STATE_UNINITIALIZED = 0,
  WIFI_STATE_INITIALIZED,
  WIFI_STATE_CONNECTING,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_DISCONNECTED,
  WIFI_STATE_AP_MODE,
  WIFI_STATE_ERROR
} wifi_state_t;

// ─────────────────────────────────────────────────────────────────────────────
// Private Data Structures
// ─────────────────────────────────────────────────────────────────────────────

typedef struct {
  wifi_state_t state;
  EventGroupHandle_t event_group;
  SemaphoreHandle_t mutex;
  esp_netif_t *sta_netif;
  esp_netif_t *ap_netif;

  // Current configuration
  char current_ssid[WIFI_SSID_MAX_LEN];
  char current_password[WIFI_PASSWORD_MAX_LEN];

  // Connection statistics
  int retry_count;
  uint32_t connect_attempts;
  uint32_t successful_connects;

  // Current network info
  esp_netif_ip_info_t ip_info;
  int8_t rssi;

  // Flags
  bool auto_reconnect_enabled;
  bool initialized;
} wifi_context_t;

// ─────────────────────────────────────────────────────────────────────────────
// Private Variables
// ─────────────────────────────────────────────────────────────────────────────

static wifi_context_t s_wifi_ctx = {0};

// ─────────────────────────────────────────────────────────────────────────────
// Private Function Declarations
// ─────────────────────────────────────────────────────────────────────────────

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
static esp_err_t wifi_init_nvs(void);
static esp_err_t wifi_init_netif(void);
static esp_err_t wifi_init_wifi_stack(void);
static esp_err_t wifi_configure_sta(const char *ssid, const char *password);
static void wifi_set_state(wifi_state_t new_state);
static wifi_state_t wifi_get_state(void);
static esp_err_t wifi_take_mutex(TickType_t timeout);
static void wifi_give_mutex(void);

// ─────────────────────────────────────────────────────────────────────────────
// Private Function Implementations
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief WiFi event handler for all WiFi and IP events
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WiFi station started");
      wifi_set_state(WIFI_STATE_CONNECTING);
      esp_wifi_connect();
      break;

    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to WiFi network");
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGW(TAG, "Disconnected from WiFi network");
      wifi_set_state(WIFI_STATE_DISCONNECTED);
      xEventGroupSetBits(s_wifi_ctx.event_group, WIFI_DISCONNECTED_BIT);

      if (s_wifi_ctx.auto_reconnect_enabled &&
          s_wifi_ctx.retry_count < CONFIG_WIFI_MAX_RETRY) {
        esp_wifi_connect();
        s_wifi_ctx.retry_count++;
        ESP_LOGI(TAG, "Retry %d/%d", s_wifi_ctx.retry_count,
                 CONFIG_WIFI_MAX_RETRY);
      } else if (s_wifi_ctx.retry_count >= CONFIG_WIFI_MAX_RETRY) {
        ESP_LOGE(TAG, "Max retries reached, connection failed");
        xEventGroupSetBits(s_wifi_ctx.event_group, WIFI_FAIL_BIT);
        wifi_set_state(WIFI_STATE_ERROR);
      }
      break;

    case WIFI_EVENT_AP_START:
      ESP_LOGI(TAG, "WiFi AP started");
      wifi_set_state(WIFI_STATE_AP_MODE);
      break;

    case WIFI_EVENT_AP_STOP:
      ESP_LOGI(TAG, "WiFi AP stopped");
      break;

    default:
      break;
    }
  } else if (event_base == IP_EVENT) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

      // Update context
      s_wifi_ctx.ip_info = event->ip_info;
      s_wifi_ctx.retry_count = 0;
      s_wifi_ctx.successful_connects++;
      wifi_set_state(WIFI_STATE_CONNECTED);

      xEventGroupSetBits(s_wifi_ctx.event_group, WIFI_CONNECTED_BIT);
    } break;

    case IP_EVENT_STA_LOST_IP:
      ESP_LOGW(TAG, "Lost IP address");
      wifi_set_state(WIFI_STATE_DISCONNECTED);
      break;

    default:
      break;
    }
  }
}

/**
 * @brief Initialize NVS flash storage
 */
static esp_err_t wifi_init_nvs(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  return ret;
}

/**
 * @brief Initialize network interface
 */
static esp_err_t wifi_init_netif(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  s_wifi_ctx.sta_netif = esp_netif_create_default_wifi_sta();
  if (s_wifi_ctx.sta_netif == NULL) {
    ESP_LOGE(TAG, "Failed to create default WiFi STA netif");
    return ESP_FAIL;
  }

  return ESP_OK;
}

/**
 * @brief Initialize WiFi stack
 */
static esp_err_t wifi_init_wifi_stack(void) {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP,
                                             &wifi_event_handler, NULL));

  return ESP_OK;
}

/**
 * @brief Configure WiFi in station mode
 */
static esp_err_t wifi_configure_sta(const char *ssid, const char *password) {
  if (!ssid) {
    ESP_LOGE(TAG, "SSID cannot be NULL");
    return ESP_ERR_INVALID_ARG;
  }

  wifi_config_t wifi_config = {0};
  strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));

  if (password) {
    strlcpy((char *)wifi_config.sta.password, password,
            sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  } else {
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Store current configuration
  strlcpy(s_wifi_ctx.current_ssid, ssid, sizeof(s_wifi_ctx.current_ssid));
  if (password) {
    strlcpy(s_wifi_ctx.current_password, password,
            sizeof(s_wifi_ctx.current_password));
  } else {
    s_wifi_ctx.current_password[0] = '\0';
  }

  return ESP_OK;
}

/**
 * @brief Thread-safe state management
 */
static void wifi_set_state(wifi_state_t new_state) {
  if (wifi_take_mutex(pdMS_TO_TICKS(1000)) == ESP_OK) {
    s_wifi_ctx.state = new_state;
    wifi_give_mutex();
  }
}

static wifi_state_t wifi_get_state(void) {
  wifi_state_t state = WIFI_STATE_ERROR;
  if (wifi_take_mutex(pdMS_TO_TICKS(1000)) == ESP_OK) {
    state = s_wifi_ctx.state;
    wifi_give_mutex();
  }
  return state;
}

/**
 * @brief Mutex management helpers
 */
static esp_err_t wifi_take_mutex(TickType_t timeout) {
  if (!s_wifi_ctx.mutex) {
    return ESP_ERR_INVALID_STATE;
  }
  return (xSemaphoreTake(s_wifi_ctx.mutex, timeout) == pdTRUE)
             ? ESP_OK
             : ESP_ERR_TIMEOUT;
}

static void wifi_give_mutex(void) {
  if (s_wifi_ctx.mutex) {
    xSemaphoreGive(s_wifi_ctx.mutex);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public Function Implementations
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t wifi_connect_sta(TickType_t timeout_ticks) {
  ESP_LOGI(TAG, "Starting WiFi connection...");

  // Use default timeout if not specified
  if (timeout_ticks == 0) {
    timeout_ticks = pdMS_TO_TICKS(WIFI_DEFAULT_TIMEOUT_MS);
  }

  esp_err_t ret = ESP_OK;

  // Initialize if not already done
  if (!s_wifi_ctx.initialized) {
    // Initialize synchronization primitives
    s_wifi_ctx.mutex = xSemaphoreCreateMutex();
    if (!s_wifi_ctx.mutex) {
      ESP_LOGE(TAG, "Failed to create mutex");
      return ESP_ERR_NO_MEM;
    }

    s_wifi_ctx.event_group = xEventGroupCreate();
    if (!s_wifi_ctx.event_group) {
      ESP_LOGE(TAG, "Failed to create event group");
      vSemaphoreDelete(s_wifi_ctx.mutex);
      return ESP_ERR_NO_MEM;
    }

    // Initialize NVS
    ret = wifi_init_nvs();
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
      goto cleanup;
    }

    // Initialize network interface
    ret = wifi_init_netif();
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
      goto cleanup;
    }

    // Initialize WiFi stack
    ret = wifi_init_wifi_stack();
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize WiFi stack: %s",
               esp_err_to_name(ret));
      goto cleanup;
    }

    // Configure default settings
    s_wifi_ctx.auto_reconnect_enabled = CONFIG_WIFI_AUTO_RECONNECT;
    s_wifi_ctx.initialized = true;
    wifi_set_state(WIFI_STATE_INITIALIZED);
  }

  // Configure WiFi with default or stored credentials
  ret = wifi_configure_sta(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure WiFi: %s", esp_err_to_name(ret));
    goto cleanup;
  }

  // Start WiFi
  s_wifi_ctx.connect_attempts++;
  s_wifi_ctx.retry_count = 0;

  ret = esp_wifi_start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
    goto cleanup;
  }

  // Wait for connection
  EventBits_t bits = xEventGroupWaitBits(s_wifi_ctx.event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdTRUE, pdFALSE, timeout_ticks);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "WiFi connected successfully");
    return ESP_OK;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(TAG, "WiFi connection failed");
    return ESP_ERR_WIFI_CONN;
  } else {
    ESP_LOGE(TAG, "WiFi connection timeout after %" PRIu32 "ms",
             pdTICKS_TO_MS(timeout_ticks));
    return ESP_ERR_TIMEOUT;
  }

cleanup:
  if (s_wifi_ctx.event_group) {
    vEventGroupDelete(s_wifi_ctx.event_group);
    s_wifi_ctx.event_group = NULL;
  }
  if (s_wifi_ctx.mutex) {
    vSemaphoreDelete(s_wifi_ctx.mutex);
    s_wifi_ctx.mutex = NULL;
  }
  s_wifi_ctx.initialized = false;
  return ret;
}

esp_err_t wifi_disconnect(void) {
  if (!s_wifi_ctx.initialized) {
    ESP_LOGW(TAG, "WiFi not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Disconnecting WiFi...");

  s_wifi_ctx.auto_reconnect_enabled = false;
  esp_err_t ret = esp_wifi_disconnect();

  if (ret == ESP_OK) {
    wifi_set_state(WIFI_STATE_DISCONNECTED);
  }

  return ret;
}

esp_err_t wifi_reconnect(TickType_t timeout_ticks) {
  ESP_LOGI(TAG, "Reconnecting WiFi...");

  if (!s_wifi_ctx.initialized) {
    return wifi_connect_sta(timeout_ticks);
  }

  // Disconnect first
  wifi_disconnect();
  vTaskDelay(pdMS_TO_TICKS(1000)); // Brief delay

  // Re-enable auto reconnect and connect
  s_wifi_ctx.auto_reconnect_enabled = CONFIG_WIFI_AUTO_RECONNECT;
  s_wifi_ctx.retry_count = 0;

  return wifi_connect_sta(timeout_ticks);
}

bool wifi_is_connected(void) {
  return (wifi_get_state() == WIFI_STATE_CONNECTED);
}

esp_err_t wifi_get_ip_info(char *ip_str, size_t ip_str_len) {
  if (!ip_str || ip_str_len < WIFI_IP_STR_MAX_LEN) {
    return ESP_ERR_INVALID_ARG;
  }

  if (!wifi_is_connected()) {
    return ESP_ERR_INVALID_STATE;
  }

  if (wifi_take_mutex(pdMS_TO_TICKS(1000)) == ESP_OK) {
    snprintf(ip_str, ip_str_len, IPSTR, IP2STR(&s_wifi_ctx.ip_info.ip));
    wifi_give_mutex();
    return ESP_OK;
  }

  return ESP_ERR_TIMEOUT;
}

int wifi_get_rssi(void) {
  if (!wifi_is_connected()) {
    return -100; // Invalid RSSI value
  }

  wifi_ap_record_t ap_info;
  esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);

  if (ret == ESP_OK) {
    s_wifi_ctx.rssi = ap_info.rssi;
    return ap_info.rssi;
  }

  return -100;
}

esp_err_t wifi_set_credentials(const char *ssid, const char *password) {
  if (!ssid) {
    return ESP_ERR_INVALID_ARG;
  }

  if (strlen(ssid) >= WIFI_SSID_MAX_LEN) {
    ESP_LOGE(TAG, "SSID too long");
    return ESP_ERR_INVALID_ARG;
  }

  if (password && strlen(password) >= WIFI_PASSWORD_MAX_LEN) {
    ESP_LOGE(TAG, "Password too long");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Setting new WiFi credentials");

  // If connected, disconnect first
  if (wifi_is_connected()) {
    wifi_disconnect();
  }

  // Configure with new credentials
  return wifi_configure_sta(ssid, password);
}

esp_err_t wifi_start_ap_mode(const char *ssid, const char *password) {
  if (!ssid) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Starting AP mode with SSID: %s", ssid);

  // Create AP netif if not exists
  if (!s_wifi_ctx.ap_netif) {
    s_wifi_ctx.ap_netif = esp_netif_create_default_wifi_ap();
    if (!s_wifi_ctx.ap_netif) {
      ESP_LOGE(TAG, "Failed to create AP netif");
      return ESP_FAIL;
    }
  }

  wifi_config_t ap_config = {
      .ap =
          {
              .channel = 1,
              .max_connection = 4,
              .authmode = password ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN,
              .beacon_interval = 100,
          },
  };

  strlcpy((char *)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid));
  ap_config.ap.ssid_len = strlen(ssid);

  if (password) {
    strlcpy((char *)ap_config.ap.password, password,
            sizeof(ap_config.ap.password));
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  return ESP_OK;
}

esp_err_t wifi_connect_deinit(void) {
  ESP_LOGI(TAG, "Deinitializing WiFi...");

  if (!s_wifi_ctx.initialized) {
    return ESP_OK;
  }

  // Stop WiFi
  esp_wifi_stop();
  esp_wifi_deinit();

  // Unregister event handlers
  esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               &wifi_event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                               &wifi_event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_LOST_IP,
                               &wifi_event_handler);

  // Cleanup netif
  if (s_wifi_ctx.sta_netif) {
    esp_netif_destroy(s_wifi_ctx.sta_netif);
    s_wifi_ctx.sta_netif = NULL;
  }

  if (s_wifi_ctx.ap_netif) {
    esp_netif_destroy(s_wifi_ctx.ap_netif);
    s_wifi_ctx.ap_netif = NULL;
  }

  // Cleanup synchronization primitives
  if (s_wifi_ctx.event_group) {
    vEventGroupDelete(s_wifi_ctx.event_group);
    s_wifi_ctx.event_group = NULL;
  }

  if (s_wifi_ctx.mutex) {
    vSemaphoreDelete(s_wifi_ctx.mutex);
    s_wifi_ctx.mutex = NULL;
  }

  // Reset context
  memset(&s_wifi_ctx, 0, sizeof(wifi_context_t));

  ESP_LOGI(TAG, "WiFi deinitialized successfully");
  return ESP_OK;
}
