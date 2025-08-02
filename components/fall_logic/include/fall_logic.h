
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fall detection event structure.
 *
 * This structure holds acceleration and event data
 * captured when a fall is detected or analyzed.
 */
typedef struct {
  uint32_t timestamp;    /*!< Timestamp (e.g., millis since boot) */
  float acceleration_x;  /*!< Acceleration in X axis (g) */
  float acceleration_y;  /*!< Acceleration in Y axis (g) */
  float acceleration_z;  /*!< Acceleration in Z axis (g) */
  float magnitude;       /*!< Acceleration magnitude (vector sum) */
  bool is_fall_detected; /*!< Flag: true if fall detected */
  uint8_t confidence;    /*!< Confidence level (0-100%) */
} fall_event_t;

#if CONFIG_FALL_LOGIC_ENABLE

/**
 * @brief Initialize fall detection logic.
 *
 * This function sets up the fall detection logic by registering shared
 * resources such as mutex and event queue. These resources must be created
 * and owned by the application layer (`app_main.c`).
 *
 * @param mutex         Handle to a mutex protecting shared sensor access
 * @param event_queue   Handle to a queue for posting fall events
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if handles are invalid
 *      - ESP_FAIL if internal error occurs
 */
esp_err_t fall_logic_init(SemaphoreHandle_t mutex, QueueHandle_t event_queue);

/**
 * @brief Start the fall detection task.
 *
 * This function launches the FreeRTOS task that reads sensor data and
 * performs fall detection continuously.
 */
void fall_logic_start(void);

/**
 * @brief Enable fall detection logic at runtime.
 *
 * This allows enabling fall detection dynamically after boot.
 */
void fall_logic_enable(void);

/**
 * @brief Disable fall detection logic at runtime.
 *
 * Use this to pause or stop the fall detection task temporarily.
 */
void fall_logic_disable(void);

/**
 * @brief Check if fall detection logic is currently active.
 *
 * @return true if enabled, false otherwise.
 */
bool fall_logic_is_enabled(void);

#else // CONFIG_FALL_LOGIC_ENABLE disabled

// Fallback macros if fall logic is not enabled in Kconfig
#define fall_logic_init(mutex, queue) (ESP_OK)
#define fall_logic_start() ((void)0)
#define fall_logic_enable() ((void)0)
#define fall_logic_disable() ((void)0)
#define fall_logic_is_enabled() (false)

#endif // CONFIG_FALL_LOGIC_ENABLE

#ifdef __cplusplus
}
#endif
