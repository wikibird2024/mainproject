#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Định nghĩa địa chỉ I2C của MPU6050
#define MPU6050_ADDR 0x68

// Thanh ghi quan trọng
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

// Cấu trúc dữ liệu cảm biến
typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
} sensor_data_t;

// Hàm khởi tạo MPU6050
esp_err_t mpu6050_init(void);

// Hàm đọc dữ liệu từ MPU6050
sensor_data_t mpu6050_read_data(void);

// Phát hiện té ngã dựa trên ngưỡng gia tốc
bool detect_fall(sensor_data_t data);

