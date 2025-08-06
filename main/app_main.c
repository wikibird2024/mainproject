/**
 * @file app_main.c
 * @brief System orchestrator and startup coordinator for the ESP32 Fall
 * Detection System.
 *
 * Initializes system components and synchronization primitives, and starts
 * application-level tasks.
 */
#include "app_main.h"
#include "buzzer.h"
#include "comm.h"
#include "debugs.h"
#include "esp_system.h"
#include "event_handler.h"
#include "fall_logic.h"
#include "led_indicator.h"
#include "sdkconfig.h"
#include "sim4g_gps.h"
#include "wifi_connect.h"
#include "user_mqtt.h"

// ─────────────────────────────────────────────────────────────────────────────
// Configuration Macros
// ─────────────────────────────────────────────────────────────────────────────
#define APP_MAX_INIT_RETRY 3
#define APP_EVENT_QUEUE_LENGTH 10
#define APP_EVENT_ITEM_SIZE sizeof(fall_event_t)

// ─────────────────────────────────────────────────────────────────────────────
// Static Shared Resources
// ─────────────────────────────────────────────────────────────────────────────
static SemaphoreHandle_t xMutex = NULL;
static QueueHandle_t xEventQueue = NULL;
static bool system_initialized = false;
static bool application_started = false;

// ─────────────────────────────────────────────────────────────────────────────
// Local Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize synchronization primitives (mutex and event queue).
 * @return ESP_OK on success, ESP_FAIL on failure
 */
static esp_err_t init_sync_primitives(void) {
  // Create mutex with retry logic
  for (int i = 0; i < APP_MAX_INIT_RETRY; ++i) {
    xMutex = xSemaphoreCreateMutex();
    if (xMutex != NULL) {
      break;
    }
    DEBUGS_LOGW("Mutex creation failed, retry %d/%d", i + 1,
                APP_MAX_INIT_RETRY);
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (xMutex == NULL) {
    DEBUGS_LOGE("Mutex creation failed after %d retries", APP_MAX_INIT_RETRY);
    return ESP_FAIL;
  }

  // Create event queue
  xEventQueue = xQueueCreate(APP_EVENT_QUEUE_LENGTH, APP_EVENT_ITEM_SIZE);
  if (xEventQueue == NULL) {
    DEBUGS_LOGE("Event queue creation failed");
    vSemaphoreDelete(xMutex);
    xMutex = NULL;
    return ESP_FAIL;
  }

  DEBUGS_LOGI("Synchronization primitives initialized successfully");
  return ESP_OK;
}

/**
 * @brief Initialize all system components and drivers.
 * @return ESP_OK on success, ESP_FAIL on failure
 */
static esp_err_t init_components(void) {
  esp_err_t ret = ESP_OK;

  // Logging system
  debugs_init();
  DEBUGS_LOGI("Debug system initialized");

  // Synchronization primitives
  ret = init_sync_primitives();
  if (ret != ESP_OK) {
    DEBUGS_LOGE("Failed to initialize synchronization primitives");
    return ret;
  }

  // Communication interfaces
  comm_init_all();
  DEBUGS_LOGI("Communication interfaces initialized");

  // WiFi connection - Simple call, all complexity handled in wifi_connect
  // module
  ret = wifi_connect_sta(0); // Use default timeout from Kconfig
  if (ret == ESP_OK) {
    DEBUGS_LOGI("WiFi connected successfully");
  } else {
    DEBUGS_LOGW("WiFi connection failed, continuing without WiFi");
  }

  // Peripherals
  buzzer_init();
  DEBUGS_LOGI("Buzzer initialized");

  led_indicator_init();
  DEBUGS_LOGI("LED indicator initialized");

  // SIM module
  sim4g_gps_init();
  sim4g_gps_set_phone_number(CONFIG_SIM4G_DEFAULT_PHONE);
  DEBUGS_LOGI("SIM4G GPS initialized with phone: %s",
              CONFIG_SIM4G_DEFAULT_PHONE);

  // Fall logic module
  fall_logic_init(xMutex, xEventQueue);
  DEBUGS_LOGI("Fall logic initialized");

  // Mqtt module 
  user_mqtt_init(CONFIG_USER_MQTT_BROKER_URI);  // Đảm bảo đã kết nối MQTT nếu có WiFi
  DEBUGS_LOGI("Mqtt initialized");

  return ESP_OK;
}

/**
 * @brief Cleanup all system resources
 */
static void cleanup_system(void) {
  // Stop application components
  if (application_started) {
    event_handler_deinit();
    application_started = false;
  }

  // Simple WiFi cleanup
  wifi_connect_deinit();

  // Cleanup sync primitives
  if (xMutex != NULL) {
    vSemaphoreDelete(xMutex);
    xMutex = NULL;
  }

  if (xEventQueue != NULL) {
    vQueueDelete(xEventQueue);
    xEventQueue = NULL;
  }

  system_initialized = false;
  DEBUGS_LOGI("System cleanup completed");
}

// ─────────────────────────────────────────────────────────────────────────────
// Public Functions
// ─────────────────────────────────────────────────────────────────────────────

esp_err_t app_system_init(void) {
  if (system_initialized) {
    DEBUGS_LOGW("System already initialized");
    return ESP_OK;
  }

  DEBUGS_LOGI("System initialization started...");

  esp_err_t ret = init_components();
  if (ret != ESP_OK) {
    DEBUGS_LOGE("Component initialization failed");
    cleanup_system();
    return ret;
  }

  system_initialized = true;
  DEBUGS_LOGI("System initialization complete.");
  return ESP_OK;
}

esp_err_t app_start_application(void) {
  if (!system_initialized) {
    DEBUGS_LOGE("System not initialized. Call app_system_init() first.");
    return ESP_ERR_INVALID_STATE;
  }

  if (application_started) {
    DEBUGS_LOGW("Application already started");
    return ESP_OK;
  }

  if (xEventQueue == NULL) {
    DEBUGS_LOGE("Event queue not available");
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t ret = fall_logic_start();
  if (ret != ESP_OK) {
    DEBUGS_LOGE("Failed to start fall logic");
    return ret;
  }

  ret = event_handler_init(xEventQueue);
  if (ret != ESP_OK) {
    DEBUGS_LOGE("Failed to initialize event handler: %s", esp_err_to_name(ret));
    return ret;
  }

  application_started = true;
  DEBUGS_LOGI("Application started successfully");
  return ESP_OK;
}

esp_err_t app_stop_application(void) {
  if (!application_started) {
    DEBUGS_LOGW("Application not started");
    return ESP_OK;
  }

  event_handler_deinit();
  application_started = false;
  DEBUGS_LOGI("Application stopped");
  return ESP_OK;
}

esp_err_t app_restart_system(void) {
  DEBUGS_LOGI("Restarting system...");

  app_stop_application();
  cleanup_system();

  esp_err_t ret = app_system_init();
  if (ret != ESP_OK) {
    DEBUGS_LOGE("System restart failed");
    return ret;
  }

  ret = app_start_application();
  if (ret != ESP_OK) {
    DEBUGS_LOGE("Application restart failed");
    return ret;
  }

  DEBUGS_LOGI("System restart completed successfully");
  return ESP_OK;
}

bool app_is_system_initialized(void) { return system_initialized; }

bool app_is_application_running(void) { return application_started; }

// Simple WiFi status check - delegate to WiFi module
bool app_is_wifi_connected(void) { return wifi_is_connected(); }

esp_err_t app_get_system_status(app_system_status_t *status) {
  if (status == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  status->system_initialized = system_initialized;
  status->application_running = application_started;
  status->mutex_available = (xMutex != NULL);
  status->event_queue_available = (xEventQueue != NULL);
  status->event_handler_initialized = event_handler_is_initialized();

  return ESP_OK;
}

// Legacy functions
SemaphoreHandle_t get_mutex(void) { return xMutex; }

QueueHandle_t get_event_queue(void) { return xEventQueue; }
