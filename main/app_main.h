
/**
 * @file app_main.h
 * @brief Public interface for system orchestration and application startup.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

/**
 * @brief Initialize the entire system (drivers, peripherals, services).
 */
void app_system_init(void);

/**
 * @brief Start main application logic (tasks, state machines, etc).
 */
void app_start_application(void);

/**
 * @brief Get handle to shared system mutex.
 * @return SemaphoreHandle_t
 */
SemaphoreHandle_t get_mutex(void);

/**
 * @brief Get handle to event queue for fall detection.
 * @return QueueHandle_t
 */
QueueHandle_t get_event_queue(void);
