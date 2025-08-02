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

// ─────────────────────────────────────────────────────────────────────────────
// Local Functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize synchronization primitives (mutex and event queue).
 */
static void init_sync_primitives(void) {
  for (int i = 0; i < APP_MAX_INIT_RETRY; ++i) {
    xMutex = xSemaphoreCreateMutex();
    if (xMutex != NULL)
      break;
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (xMutex == NULL) {
    DEBUGS_LOGE("Mutex creation failed. Restarting...");
    esp_restart();
  }

  xEventQueue = xQueueCreate(APP_EVENT_QUEUE_LENGTH, APP_EVENT_ITEM_SIZE);
  if (xEventQueue == NULL) {
    DEBUGS_LOGE("Event queue creation failed. Restarting...");
    esp_restart();
  }
}

/**
 * @brief Initialize all system components and drivers.
 */
static void init_components(void) {
  // Logging system
  debugs_init();

  // Synchronization
  init_sync_primitives();

  // Communication interfaces
  comm_init_all(); // Unified UART/I2C init (you can implement this)

  // Peripherals
  buzzer_init();
  led_indicator_init();

  // SIM module
  sim4g_gps_init();
  sim4g_gps_set_phone_number(CONFIG_SIM4G_DEFAULT_PHONE);

  // Fall logic module
  fall_logic_init(xMutex, xEventQueue);

  DEBUGS_LOGI("System initialization complete.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Public Functions
// ─────────────────────────────────────────────────────────────────────────────

void app_system_init(void) {
  DEBUGS_LOGI("System initialization started...");
  init_components();
}

void app_start_application(void) {
  fall_logic_start();
  event_handler_start();
}

SemaphoreHandle_t get_mutex(void) { return xMutex; }

QueueHandle_t get_event_queue(void) { return xEventQueue; }
