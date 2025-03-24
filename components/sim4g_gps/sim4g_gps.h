#pragma once

#include <stdio.h>

// Cấu trúc dữ liệu GPS
typedef struct {
    char latitude[20];
    char longitude[20];
    char timestamp[20];
} gps_data_t;

// Khởi tạo module SIM 4G GPS
void sim4g_gps_init();

// Cấu hình số điện thoại nhận tin nhắn
void sim4g_gps_set_phone_number(const char *number);

// Lấy vị trí GPS hiện tại
gps_data_t sim4g_gps_get_location();

// Gửi tin nhắn SMS khi phát hiện té ngã
void send_fall_alert_sms(gps_data_t location);

