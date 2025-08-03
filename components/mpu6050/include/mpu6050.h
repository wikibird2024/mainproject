/**
 * @file mpu6050.h
 * @brief Interface for MPU6050 6-axis accelerometer and gyroscope sensor.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>

// Default I2C address and MPU6050 register map
#define MPU6050_ADDR 0x68         ///< Default I2C address of MPU6050
#define MPU6050_PWR_MGMT_1 0x6B   ///< Power management register
#define MPU6050_ACCEL_XOUT_H 0x3B ///< Starting register for accel/gyro data

/**
 * @brief Struct representing raw sensor data from MPU6050.
 *
 * Units:
 * - Acceleration in g (gravitational force)
 * - Angular velocity in degrees/second
 */
typedef struct {
  float accel_x; ///< Acceleration along X-axis (g)
  float accel_y; ///< Acceleration along Y-axis (g)
  float accel_z; ///< Acceleration along Z-axis (g)
  float gyro_x;  ///< Rotation rate around X-axis (deg/s)
  float gyro_y;  ///< Rotation rate around Y-axis (deg/s)
  float gyro_z;  ///< Rotation rate around Z-axis (deg/s)
} sensor_data_t;

/**
 * @brief Initialize MPU6050 by waking it from sleep and setting default config.
 *
 * This function assumes the device is connected via I2C to the default address.
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL or other esp_err_t codes on error
 */
esp_err_t mpu6050_init(void);

/**
 * @brief Put MPU6050 into low-power sleep mode.
 */
esp_err_t mpu6050_deinit(void);

/**
 * @brief Read all accelerometer and gyroscope data from MPU6050.
 *
 * Reads 14 consecutive bytes from MPU6050 starting at ACCEL_XOUT_H.
 *
 * @param[out] data Pointer to struct to store sensor values
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if data is NULL
 *      - ESP_FAIL on communication error
 */
esp_err_t mpu6050_read_data(sensor_data_t *data);

#ifdef __cplusplus
}
#endif
