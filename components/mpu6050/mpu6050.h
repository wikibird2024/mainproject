#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Địa chỉ I2C và thanh ghi quan trọng của MPU6050
#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

// Dữ liệu gia tốc và gyro (chuẩn hóa)
typedef struct {
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
} sensor_data_t;

esp_err_t mpu6050_init(void);                  // Khởi tạo MPU6050
sensor_data_t mpu6050_read_data(void);         // Đọc dữ liệu cảm biến
bool detect_fall(sensor_data_t data);          // Phát hiện té ngã
