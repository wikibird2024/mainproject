#ifndef _DATA_MANAGER_TYPES_H_
#define _DATA_MANAGER_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Data structure for GPS data.
 *
 * This struct is now defined here to make the data_manager component
 * standalone.
 */
typedef struct {
  float latitude;
  float longitude;
  char timestamp[24];
  bool has_gps_fix;
} gps_data_t;

/**
 * @brief Data structure containing all device state.
 *
 * This serves as the Single Source of Truth for the entire system.
 * All modules get data from here to ensure consistency.
 */
typedef struct {
  // Basic Information
  char device_id[32];
  uint64_t timestamp_ms;

  // Sensor State
  bool fall_detected;

  // Connection State
  bool wifi_connected;
  bool mqtt_connected;
  bool sim_registered;

  // GPS Data
  gps_data_t gps_data;

  // Add other data fields as needed
} device_state_t;

#ifdef __cplusplus
}
#endif

#endif // _DATA_MANAGER_TYPES_H_
