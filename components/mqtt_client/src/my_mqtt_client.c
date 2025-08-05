
/**
 * @file my_mqtt_client.c
 * @brief MQTT client for publishing device data as JSON.
 *
 * Initializes the MQTT client using settings from Kconfig,
 * builds JSON from structured device data using `json_wrapper`,
 * and publishes to MQTT broker using ESP-IDF APIs.
 */

#if CONFIG_MQTT_CLIENT_ENABLE

#include "common/device_data.h"       // shared struct
#include "my_mqtt_client.h"           // this component's API
#include "json_wrapper.h"             // JSON builder
#include "debugs.h"                   // logging abstraction

#include "esp_log.h"
#include "mqtt_client.h"
#include <stdlib.h>
#include <string.h>

/* ───── CONFIGURABLE PARAMETERS FROM Kconfig ─────────────────────────────── */
#define MQTT_URI     CONFIG_MQTT_BROKER_URI
#define MQTT_TOPIC   CONFIG_MQTT_PUB_TOPIC

/* ───── INTERNAL MQTT CLIENT HANDLE ──────────────────────────────────────── */
static esp_mqtt_client_handle_t client = NULL;

/* ───── PUBLIC FUNCTION IMPLEMENTATIONS ──────────────────────────────────── */

void mqtt_client_start(void) {
#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGI("Initializing MQTT client...");
#endif

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (!client) {
#if CONFIG_DEBUGS_ENABLE_LOG
        DEBUGS_LOGE("Failed to initialize MQTT client");
#endif
        return;
    }

    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
#if CONFIG_DEBUGS_ENABLE_LOG
        DEBUGS_LOGE("Failed to start MQTT client: %s", esp_err_to_name(err));
#endif
        return;
    }

#if CONFIG_DEBUGS_ENABLE_LOG
    DEBUGS_LOGI("MQTT client started, broker: %s", MQTT_URI);
#endif
}

void mqtt_client_publish_data(const device_data_t *data) {
    if (!data) {
#if CONFIG_DEBUGS_ENABLE_LOG
        DEBUGS_LOGW("Attempted to publish NULL device data");
#endif
        return;
    }

    char *json_str = json_wrapper_build_device_json(data);
    if (!json_str) {
#if CONFIG_DEBUGS_ENABLE_LOG
        DEBUGS_LOGE("Failed to build JSON string from device data");
#endif
        return;
    }

    mqtt_client_publish_json(MQTT_TOPIC, json_str);
    free(json_str);
}

void mqtt_client_publish_json(const char *topic, const char *json_str) {
    if (!client) {
#if CONFIG_DEBUGS_ENABLE_LOG
        DEBUGS_LOGW("MQTT client not initialized");
#endif
        return;
    }

    if (!topic || !json_str) {
#if CONFIG_DEBUGS_ENABLE_LOG
        DEBUGS_LOGW("NULL topic or JSON string passed to publish");
#endif
        return;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, json_str, 0, 1, 0);
#if CONFIG_DEBUGS_ENABLE_LOG
    if (msg_id >= 0) {
        DEBUGS_LOGI("MQTT published (ID=%d) → topic: %s", msg_id, topic);
    } else {
        DEBUGS_LOGE("MQTT publish failed → topic: %s", topic);
    }
#endif
}

#endif // CONFIG_MQTT_CLIENT_ENABLE
