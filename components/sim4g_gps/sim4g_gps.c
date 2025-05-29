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

// Mutex bảo vệ truy cập GPS
static SemaphoreHandle_t gps_mutex = NULL;

// Số điện thoại nhận cảnh báo (private)
static char phone_number[16] = DEFAULT_PHONE;

// Lưu vị trí GPS cuối cùng (private)
static gps_data_t last_location = {0};

// Khởi tạo GPS: bật GPS và cấu hình
void sim4g_gps_init(void) {
    char response[64] = {0};

    if (gps_mutex == NULL) {
        gps_mutex = xSemaphoreCreateMutex();
        if (gps_mutex == NULL) {
            ERROR("Failed to create GPS mutex");
            return;
        }
    }

    comm_uart_send_command("AT+QGPSCFG=\"autogps\",1", NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    comm_uart_send_command("AT+QGPS=1", response, sizeof(response));
    vTaskDelay(pdMS_TO_TICKS(500));

    if (strstr(response, "OK") != NULL) {
        INFO("GPS initialized successfully");
    } else {
        ERROR("GPS initialization failed");
    }
}

// Thiết lập số điện thoại nhận SMS cảnh báo
void sim4g_gps_set_phone_number(const char *number) {
    if (number == NULL) {
        ERROR("Invalid phone number (NULL)");
        return;
    }
    if (strlen(number) >= sizeof(phone_number)) {
        ERROR("Phone number too long");
        return;
    }
    strncpy(phone_number, number, sizeof(phone_number) - 1);
    phone_number[sizeof(phone_number) - 1] = '\0';
    INFO("Phone number set to: %s", phone_number);
}

// Lấy vị trí GPS hiện tại
gps_data_t sim4g_gps_get_location(void) {
    char response[128] = {0};
    memset(&last_location, 0, sizeof(last_location));
    last_location.valid = false;

    if (gps_mutex == NULL) {
        ERROR("GPS mutex not initialized");
        return last_location;
    }

    if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        comm_uart_send_command("AT+QGPSLOC?", response, sizeof(response));
        vTaskDelay(pdMS_TO_TICKS(500));

        // Định dạng dữ liệu mẫu: +QGPSLOC: 20230515123045.000,10.762622,106.660172,10.0,0.0,0.0,1
        // Lấy timestamp, latitude, longitude
        if (sscanf(response, "+QGPSLOC: %19[^,],%19[^,],%19[^,],%*s",
                   last_location.timestamp,
                   last_location.latitude,
                   last_location.longitude) == 3) {
            last_location.valid = true;
            INFO("GPS location: Lat=%s, Lon=%s, Time=%s",
                 last_location.latitude,
                 last_location.longitude,
                 last_location.timestamp);
        } else {
            ERROR("Failed to parse GPS location response");
        }

        xSemaphoreGive(gps_mutex);
    } else {
        ERROR("Timeout acquiring GPS mutex");
    }

    return last_location;
}

// Gửi SMS cảnh báo té ngã (thực hiện gửi inline)
static void send_fall_alert_sms_inline(const gps_data_t *location) {
    if (location == NULL || !location->valid) {
        ERROR("Invalid GPS location data");
        return;
    }

    char sms[256] = {0};
    char cmd[64] = {0};

    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phone_number);
    comm_uart_send_command(cmd, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    snprintf(sms, sizeof(sms),
             "Fall detected!\nLocation:\nLat: %s\nLon: %s\nTime: %s",
             location->latitude,
             location->longitude,
             location->timestamp);
    comm_uart_send_command(sms, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Kết thúc tin nhắn bằng Ctrl+Z
    comm_uart_send_command("\x1A", NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(2000));

    INFO("SMS sent: %s", sms);
}

// Task gửi SMS chạy nền
static void sms_task(void *param) {
    gps_data_t *location = (gps_data_t *)param;
    if (location != NULL) {
        send_fall_alert_sms_inline(location);
        vPortFree(location);
    } else {
        ERROR("sms_task received NULL param");
    }
    vTaskDelete(NULL);
}

// Tạo task gửi SMS cảnh báo té ngã không làm blocking
void send_fall_alert_sms(const gps_data_t *location) {
    if (location == NULL || !location->valid) {
        ERROR("Invalid GPS location input for SMS");
        return;
    }

    gps_data_t *copy = pvPortMalloc(sizeof(gps_data_t));
    if (copy == NULL) {
        ERROR("Memory allocation failed for SMS task");
        return;
    }

    *copy = *location;
    BaseType_t result = xTaskCreate(sms_task, "sms_task", 4096, copy, 5, NULL);
    if (result != pdPASS) {
        ERROR("Failed to create sms_task");
        vPortFree(copy);
    }
}
