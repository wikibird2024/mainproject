#ifndef JSON_WRAPPER_H
#define JSON_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "data_manager_types.h"

// This is the function you provided, we can rename it.
char *json_wrapper_create_status_payload(void);

// This is the new function for a fall alert.
char *json_wrapper_create_alert_payload(void);

#ifdef __cplusplus
}
#endif

#endif // JSON_WRAPPER_H
