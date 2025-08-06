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

    // 1. Data Manager - Cần phải chạy để các module khác ghi log
    ret = data_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Data Manager");
        return ret; // Lỗi nghiêm trọng, dừng hệ thống
    }
    ESP_LOGI(TAG, "Data Manager initialized");

    // 2. Event Handler - Cần để xử lý sự kiện
    ret = event_handler_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Event Handler");
        return ret; // Lỗi nghiêm trọng
    }
    ESP_LOGI(TAG, "Event Handler initialized");

    // 3. Communication interfaces - Nền tảng cho tất cả giao tiếp
    ret = comm_init_all();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Communication interfaces");
        return ret; // Lỗi nghiêm trọng
    }
    ESP_LOGI(TAG, "Communication interfaces initialized");

    // 4. Các ngoại vi - Phụ thuộc vào Comm
    buzzer_init();
    ESP_LOGI(TAG, "Buzzer initialized");
    
    led_indicator_init();
    ESP_LOGI(TAG, "LED indicator initialized");

    // 5. Fall logic - Cần thiết cho chức năng chính
    ret = fall_logic_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Fall Logic");
        return ret; // Lỗi nghiêm trọng
    }
    ESP_LOGI(TAG, "Fall Logic initialized");
    
    // 6. WiFi và MQTT - Có thể bị lỗi nhưng hệ thống vẫn chạy
    ret = wifi_connect_sta(0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi connection failed, continuing without WiFi");
        // Không return, hệ thống tiếp tục
    } else {
        ESP_LOGI(TAG, "WiFi connected successfully");
    }

    ret = user_mqtt_init(CONFIG_USER_MQTT_BROKER_URI);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MQTT");
        // MQTT là giao tiếp chính. Bạn có thể coi đây là lỗi nghiêm trọng.
        // Hoặc ghi log và tiếp tục nếu bạn có phương án dự phòng khác.
        // Hiện tại, ta sẽ coi đây là lỗi nghiêm trọng.
        return ret;
    }
    ESP_LOGI(TAG, "MQTT initialized");
    
    // 7. SIM4G và GPS - Không quan trọng bằng WiFi/MQTT
    ret = sim4g_gps_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SIM4G GPS. Continuing with other services...");
        // Đây là điểm mấu chốt: KHÔNG return.
        // Hệ thống đã ghi nhận lỗi và sẽ tiếp tục.
    } else {
        sim4g_gps_set_phone_number(CONFIG_SIM4G_DEFAULT_PHONE);
        ESP_LOGI(TAG, "SIM4G GPS initialized with phone: %s", CONFIG_SIM4G_DEFAULT_PHONE);
    }

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
