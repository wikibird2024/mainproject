#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DEFAULT_PHONE "+84901234567"
#define TAG "SIM4G_GPS"

static char phone_number[16] = DEFAULT_PHONE;
static gps_data_t last_location = {0};

void sim4g_gps_init(void) {
    char response[64];
    comm_send_command("AT+QGPS=1", response, sizeof(response));
    vTaskDelay(pdMS_TO_TICKS(500));

    if (strstr(response, "OK")) {
        debugs_log(TAG, "GPS initialized.");
    } else {
        debugs_log(TAG, "GPS initialization failed!");
    }
}

void sim4g_gps_set_phone_number(const char *number) {
    if (number && strlen(number) < sizeof(phone_number)) {
        strncpy(phone_number, number, sizeof(phone_number) - 1);
        phone_number[sizeof(phone_number) - 1] = '\0';
        debugs_log(TAG, "Phone set: %s", phone_number);
    }
}

gps_data_t sim4g_gps_get_location(void) {
    char response[128];
    memset(&last_location, 0, sizeof(last_location));
    last_location.valid = false;

    comm_send_command("AT+QGPSLOC?", response, sizeof(response));
    vTaskDelay(pdMS_TO_TICKS(500));

    if (sscanf(response, "+QGPSLOC: %[^,],%[^,],%[^,],%*s",
               last_location.timestamp,
               last_location.latitude,
               last_location.longitude) == 3) {
        last_location.valid = true;
        debugs_log(TAG, "Location: %s, %s at %s",
                   last_location.latitude,
                   last_location.longitude,
                   last_location.timestamp);
    } else {
        debugs_log(TAG, "Failed to get location!");
    }

    return last_location;
}

static void sms_task(void *param) {
    gps_data_t *location = (gps_data_t *)param;
    char sms[128];
    snprintf(sms, sizeof(sms),
             "Fall detected! Location: %s, %s",
             location->latitude, location->longitude);

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);
    comm_send_command(cmd, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    comm_send_command(sms, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    comm_send_command("\x1A", NULL, 0);  // kết thúc tin nhắn
    vTaskDelay(pdMS_TO_TICKS(500));

    debugs_log(TAG, "SMS sent: %s", sms);
    vPortFree(location);
    vTaskDelete(NULL);
}

void send_fall_alert_sms(gps_data_t location) {
    if (!location.valid) {
        debugs_log(TAG, "Dữ liệu GPS không hợp lệ. Không gửi SMS.");
        return;
    }

    gps_data_t *copy = pvPortMalloc(sizeof(gps_data_t));
    if (copy) {
        *copy = location;
        xTaskCreate(sms_task, "sms_task", 2048, copy, 5, NULL);
    } else {
        debugs_log(TAG, "Không thể cấp phát bộ nhớ cho SMS task.");
    }
}

