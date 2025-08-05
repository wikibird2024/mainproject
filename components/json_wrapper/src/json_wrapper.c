
#include "json_wrapper.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

char *json_wrapper_build_device_json(const device_data_t *data) {
    if (!data) return NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "device_id", data->device_id);
    cJSON_AddBoolToObject(root, "fall_detected", data->fall_detected);
    cJSON_AddStringToObject(root, "timestamp", data->timestamp);
    cJSON_AddNumberToObject(root, "latitude", data->latitude);
    cJSON_AddNumberToObject(root, "longitude", data->longitude);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root); // free tree

    return json_str; // caller must free
}
