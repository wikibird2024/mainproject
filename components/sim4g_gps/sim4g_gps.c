#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define DEFAULT_PHONE "+84901234567"
#define TAG "SIM4G_GPS"

// Mutex cho lay GPS
static SemaphoreHandle_t gps_mutex = NULL;

// Sdt va du lieu GPS cuoi cung
static char phone_number[16] = DEFAULT_PHONE;
static gps_data_t last_location = {0};

// Khoi tao GPS
void sim4g_gps_init(void) {
    char response[64];
    char cmd[64];  // 
    if (gps_mutex == NULL) {
        gps_mutex = xSemaphoreCreateMutex();
    }

    comm_uart_send_command("AT+QGPSCFG=\"autogps\",1", NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    comm_uart_send_command("AT+QGPS=1", response, sizeof(response));
    vTaskDelay(pdMS_TO_TICKS(500));

    if (strstr(response, "OK")) {
        INFO("GPS initialized.");
    } else {
        ERROR("GPS initialization failed!");
    }
}

// Set SDT
void sim4g_gps_set_phone_number(const char *number) {
    if (number && strlen(number) < sizeof(phone_number)) {
        strncpy(phone_number, number, sizeof(phone_number) - 1);
        phone_number[sizeof(phone_number) - 1] = '\0';
        INFO("Phone set: %s", phone_number);
    }
}

// Lay tin hieu  GPS
gps_data_t sim4g_gps_get_location(void) {
    char response[128];
    memset(&last_location, 0, sizeof(last_location));
    last_location.valid = false;

    if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        comm_uart_send_command("AT+QGPSLOC?", response, sizeof(response));
        vTaskDelay(pdMS_TO_TICKS(500));

        //kiem tra gia tri tra ve 
        if (sscanf(response, "+QGPSLOC: %[^,],%[^,],%[^,],%*s",
                   last_location.timestamp,
                   last_location.latitude,
                   last_location.longitude) == 3) {
            last_location.valid = true;
            INFO("Location: %s, %s at %s",
                 last_location.latitude,
                 last_location.longitude,
                 last_location.timestamp);
        } else {
            ERROR("Failed to get location!");
        }

        xSemaphoreGive(gps_mutex);
    } else {
        ERROR("Unable to take mutex for GPS access");
    }

    return last_location;
}

// Ham gui tin nhan tu vi tri GPS
void send_fall_alert_sms_inline(const gps_data_t *location) {
    char sms[256];
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);
    comm_uart_send_command(cmd, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    // gan noi dung itn nhan
    snprintf(sms, sizeof(sms),
                "Fall detected!\nLocation:\nLat: %s\nLon: %s\nTime: %s",
             location->latitude,
             location->longitude,
             location->timestamp);

    comm_uart_send_command(sms, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    comm_uart_send_command("\x1A", NULL, 0);  // Ket thuc tin nhan
    vTaskDelay(pdMS_TO_TICKS(2000));

    INFO("SMS sent: %s", sms);
}

// Task gui SMS chay nen
static void sms_task(void *param) {
    gps_data_t *location = (gps_data_t *)param;
    send_fall_alert_sms_inline(location);
    vPortFree(location);
    vTaskDelete(NULL);
}

// Tạo task gửi SMS khi có té ngã
void send_fall_alert_sms(const gps_data_t *location) {
    gps_data_t *copy = pvPortMalloc(sizeof(gps_data_t));
    if (!copy) {
        ERROR("Không cấp phát bộ nhớ.");
        return;
    }

    *copy = *location;
    if (xTaskCreate(sms_task, "sms_task", 4096, copy, 5, NULL) != pdPASS) {
        ERROR("Không thể tạo SMS task.");
        vPortFree(copy);
    }
}
