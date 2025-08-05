#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Data structure representing a device's status and location.
 */
typedef struct {
    char device_id[32];
    bool fall_detected;
    char timestamp[20];
    double latitude;
    double longitude;
} device_data_t;

#ifdef __cplusplus
}
#endif
