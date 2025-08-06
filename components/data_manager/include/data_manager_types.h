#ifndef _DATA_MANAGER_TYPES_H_
#define _DATA_MANAGER_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "sim4g_gps.h"

/**
 * @brief Data structure containing all device state.
 *
 * This serves as the Single Source of Truth for the entire system.
 * All modules get data from here to ensure consistency.
 */
typedef struct {
    // Basic Information
    char device_id[32];
    uint64_t timestamp_ms; // Using 64-bit for millisecond timestamp

    // Sensor State
    bool fall_detected;

    // Connection State
    bool wifi_connected;
    bool mqtt_connected;
    bool sim_registered;

    // GPS Data
    sim4g_gps_data_t gps_data; // MODIFIED: Using the new GPS data struct

    // Add other data fields as needed
} device_state_t;

#ifdef __cplusplus
}
#endif

#endif // _DATA_MANAGER_TYPES_H_
