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
    float accel_x;  ///< Gia tốc trục X (đơn vị g)
    float accel_y;  ///< Gia tốc trục Y (đơn vị g)
    float accel_z;  ///< Gia tốc trục Z (đơn vị g)
    float gyro_x;   ///< Tốc độ góc trục X (đơn vị độ/giây)
    float gyro_y;   ///< Tốc độ góc trục Y (đơn vị độ/giây)
    float gyro_z;   ///< Tốc độ góc trục Z (đơn vị độ/giây)
} sensor_data_t;

/**
 * @brief Khởi tạo cảm biến MPU6050 (bỏ chế độ ngủ)
 * @return ESP_OK nếu thành công, lỗi khác nếu thất bại
 */
esp_err_t mpu6050_init(void);

/**
 * @brief Đọc dữ liệu gia tốc và gyro từ MPU6050
 * @param[out] data Pointer đến vùng nhớ chứa dữ liệu cảm biến
 * @return ESP_OK nếu đọc thành công, lỗi khác nếu thất bại
 */
esp_err_t mpu6050_read_data(sensor_data_t *data);

/**
 * @brief Phát hiện té ngã dựa trên dữ liệu gia tốc
 * @param data Dữ liệu cảm biến đã đọc
 * @return true nếu phát hiện té ngã, false nếu không
 */
bool detect_fall(sensor_data_t data);
