/**
 * @file comm_log.h
 * @brief Logging macros for communication module
 */

#ifndef COMM_LOG_H
#define COMM_LOG_H

#include "esp_log.h"

#define COMM_LOG_TAG "COMM"

// Logging macros
#define COMM_LOGI(fmt, ...) ESP_LOGI(COMM_LOG_TAG, fmt, ##__VA_ARGS__)
#define COMM_LOGE(fmt, ...) ESP_LOGE(COMM_LOG_TAG, fmt, ##__VA_ARGS__)

#endif // COMM_LOG_H
