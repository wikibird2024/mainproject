
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "common/device_data.h" // for device_data_t

/**
 * @brief Convert device data to JSON string.
 * Caller must free the returned string.
 */
char *json_wrapper_build_device_json(const device_data_t *data);

#ifdef __cplusplus
}
#endif
