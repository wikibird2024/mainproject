#pragma once

#include <stdio.h>
#include <stdbool.h>
// Dữ liệu GPS: vĩ độ, kinh độ, thời gian (UTC)
typedef struct {
    char latitude[20];
    char longitude[20];
    char timestamp[20];
    bool valid;
} gps_data_t;

void sim4g_gps_init(void);                         // Bật GPS (AT+QGPS=1)
void sim4g_gps_set_phone_number(const char *num);  // Thiết lập số điện thoại nhận cảnh báo
gps_data_t sim4g_gps_get_location(void);           // Lấy vị trí GPS (AT+QGPSLOC?)
void send_fall_alert_sms(gps_data_t location);     // Gửi SMS cảnh báo té ngã (dùng FreeRTOS task)
