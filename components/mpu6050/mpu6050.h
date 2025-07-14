/**
 * @file mpu6050.h
 * @brief MPU6050 sensor interface
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>

// MPU6050 I2C address and register definitions
#define MPU6050_ADDR          0x68
#define MPU6050_PWR_MGMT_1    0x6B
#define MPU6050_ACCEL_XOUT_H  0x3B

/**
 * @brief Sensor data structure for MPU6050 accelerometer and gyroscope.
 */
typedef struct {
    float accel_x;  ///< Acceleration X-axis (g)
    float accel_y;  ///< Acceleration Y-axis (g)
    float accel_z;  ///< Acceleration Z-axis (g)
    float gyro_x;   ///< Angular velocity X-axis (deg/s)
    float gyro_y;   ///< Angular velocity Y-axis (deg/s)
    float gyro_z;   ///< Angular velocity Z-axis (deg/s)
} sensor_data_t;

/**
 * @brief Initialize the MPU6050 sensor (wake from sleep mode).
 *
 * @return ESP_OK on success, or appropriate error code on failure.
 */
esp_err_t mpu6050_init(void);

/**
 * @brief Read accelerometer and gyroscope data from MPU6050.
 *
 * @param[out] data Pointer to struct to store the sensor readings.
 * @return ESP_OK on success, or appropriate error code on failure.
 */
esp_err_t mpu6050_read_data(sensor_data_t *data);

/**
 * @brief Detect fall event based on acceleration deviation.
 *
 * @param[in] data Pointer to the last sensor readings.
 * @return true if fall is detected, false otherwise.
 */
bool mpu6050_detect_fall(const sensor_data_t *data);

#ifdef __cplusplus
}
#endif
