/**
 * @file app_main.c
 * @brief System orchestrator and startup coordinator for the ESP32 Fall
 * Detection System.
 *
 * This file handles the top-level initialization and coordination of all
 * components, following a decoupled and modular architecture.
 */
#include "app_main.h"
#include "esp_log.h"
#include "buzzer.h"
#include "comm.h"
#include "data_manager.h"
#include "event_handler.h"
#include "fall_logic.h"
#include "led_indicator.h"
#include "sdkconfig.h"
#include "sim4g_gps.h"
#include "wifi_connect.h"
#include "user_mqtt.h"

static const char *TAG = "APP_MAIN";

// ─────────────────────────────────────────────────────────────────────────────
// Static Shared Resources
// ─────────────────────────────────────────────────────────────────────────────
static bool system_initialized = false;
static bool application_started = false;

// ─────────────────────────────────────────────────────────────────────────────
// Local Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initializes all system components and drivers.
 * @return ESP_OK on success, ESP_FAIL on failure.
 */
static esp_err_t init_components(void) {
  esp_err_t ret;

  // 1. Data Manager must be initialized first to be used by other modules.
  ret = data_manager_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize Data Manager");
    return ret;
  }
  ESP_LOGI(TAG, "Data Manager initialized");
  
  // 2. Event Handler (Initialized after Data Manager)
  ret = event_handler_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize Event Handler");
    return ret;
  }
  ESP_LOGI(TAG, "Event Handler initialized");
  
  // 3. Peripherals
  buzzer_init();
  ESP_LOGI(TAG, "Buzzer initialized");

  led_indicator_init();
  ESP_LOGI(TAG, "LED indicator initialized");

  // 4. Fall logic module
  ret = fall_logic_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize Fall Logic");
    return ret;
  }
  ESP_LOGI(TAG, "Fall Logic initialized");
  
  // 5. SIM4G and GPS
  // The new sim4g_gps_init() now handles its own AT driver initialization
  ret = sim4g_gps_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SIM4G GPS");
    return ret;
  }
  sim4g_gps_set_phone_number(CONFIG_SIM4G_DEFAULT_PHONE);
  ESP_LOGI(TAG, "SIM4G GPS initialized with phone: %s", CONFIG_SIM4G_DEFAULT_PHONE);
  
  // 6. Communication interfaces
  comm_init_all();
  ESP_LOGI(TAG, "Communication interfaces initialized");
  
  // 7. WiFi connection
  ret = wifi_connect_sta(0);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "WiFi connected successfully");
  } else {
    ESP_LOGW(TAG, "WiFi connection failed, continuing without WiFi");
  }

  // 8. Mqtt module
  ret = user_mqtt_init(CONFIG_USER_MQTT_BROKER_URI);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize MQTT");
    return ret;
  }
  ESP_LOGI(TAG, "MQTT initialized");

  return ESP_OK;
}

/**
 * @brief Cleans up all system resources.
 */
static void cleanup_system(void) {
  if (application_started) {
    event_handler_deinit();
    application_started = false;
  }

  wifi_connect_deinit();
  data_manager_deinit();

  system_initialized = false;
  ESP_LOGI(TAG, "System cleanup completed");
}

// ─────────────────────────────────────────────────────────────────────────────
// Public Functions
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t app_system_init(void) {
  if (system_initialized) {
    ESP_LOGW(TAG, "System already initialized");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "System initialization started...");

  esp_err_t ret = init_components();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Component initialization failed");
    cleanup_system();
    return ret;
  }

  system_initialized = true;
  ESP_LOGI(TAG, "System initialization complete.");
  return ESP_OK;
}

esp_err_t app_start_application(void) {
  if (!system_initialized) {
    ESP_LOGE(TAG, "System not initialized. Call app_system_init() first.");
    return ESP_ERR_INVALID_STATE;
  }

  if (application_started) {
    ESP_LOGW(TAG, "Application already started");
    return ESP_OK;
  }
  
  // The new sim4g_gps_init() no longer needs to be called here.
  // It is now called once in init_components.

  // Start the fall logic task
  esp_err_t ret = fall_logic_start();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start fall logic");
    return ret;
  }

  application_started = true;
  ESP_LOGI(TAG, "Application started successfully");
  return ESP_OK;
}

esp_err_t app_stop_application(void) {
  if (!application_started) {
    ESP_LOGW(TAG, "Application not started");
    return ESP_OK;
  }

  event_handler_deinit();
  application_started = false;
  ESP_LOGI(TAG, "Application stopped");
  return ESP_OK;
}

esp_err_t app_restart_system(void) {
  ESP_LOGI(TAG, "Restarting system...");

  app_stop_application();
  cleanup_system();

  esp_err_t ret = app_system_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "System restart failed");
    return ret;
  }

  ret = app_start_application();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Application restart failed");
    return ret;
  }

  ESP_LOGI(TAG, "System restart completed successfully");
  return ESP_OK;
}

bool app_is_system_initialized(void) { return system_initialized; }

bool app_is_application_running(void) { return application_started; }

bool app_is_wifi_connected(void) { return wifi_is_connected(); }
