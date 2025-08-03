
/**
 * @file mpu6050.c
 * @brief MPU6050 I2C driver for ESP32 – read accelerometer and gyroscope.
 */

#include "mpu6050.h"
#include "comm.h"
#include "debugs.h"
#include <math.h>
#include <string.h>

// Scale factors from datasheet
static const float ACCEL_SCALE_FACTOR = 16384.0f; // LSB/g
static const float GYRO_SCALE_FACTOR = 131.0f;    // LSB/(°/s)

#define COMBINE_BYTES(msb, lsb) ((int16_t)(((msb) << 8) | (lsb)))

/**
 * @brief Initialize MPU6050 by clearing the sleep bit.
 */
esp_err_t mpu6050_init(void) {
  DEBUGS_LOGI("Initializing MPU6050...");

  esp_err_t err = comm_i2c_write_byte(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
  if (err != ESP_OK) {
    DEBUGS_LOGE("MPU6050 I2C init failed: %s", esp_err_to_name(err));
    return err;
  }

  DEBUGS_LOGI("MPU6050 initialized successfully.");
  return ESP_OK;
}

/**
 * @brief Deinitialize MPU6050 by setting sleep mode.
 */
esp_err_t mpu6050_deinit(void) {
  DEBUGS_LOGI("Putting MPU6050 into sleep mode...");

  esp_err_t err = comm_i2c_write_byte(MPU6050_ADDR, MPU6050_PWR_MGMT_1,
                                      0x40); // Set sleep bit
  if (err != ESP_OK) {
    DEBUGS_LOGE("Failed to write sleep mode: %s", esp_err_to_name(err));
    return err;
  }

  DEBUGS_LOGI("MPU6050 entered sleep mode.");
  return ESP_OK;
}

/**
 * @brief Read scaled accelerometer and gyroscope values.
 */
esp_err_t mpu6050_read_data(sensor_data_t *data) {
  if (data == NULL) {
    DEBUGS_LOGE("Null pointer passed to mpu6050_read_data");
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t raw[14] = {0};
  esp_err_t err =
      comm_i2c_read(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, raw, sizeof(raw));
  if (err != ESP_OK) {
    DEBUGS_LOGE("MPU6050 I2C read failed: %s", esp_err_to_name(err));
    return err;
  }

  data->accel_x = COMBINE_BYTES(raw[0], raw[1]) / ACCEL_SCALE_FACTOR;
  data->accel_y = COMBINE_BYTES(raw[2], raw[3]) / ACCEL_SCALE_FACTOR;
  data->accel_z = COMBINE_BYTES(raw[4], raw[5]) / ACCEL_SCALE_FACTOR;

  data->gyro_x = COMBINE_BYTES(raw[8], raw[9]) / GYRO_SCALE_FACTOR;
  data->gyro_y = COMBINE_BYTES(raw[10], raw[11]) / GYRO_SCALE_FACTOR;
  data->gyro_z = COMBINE_BYTES(raw[12], raw[13]) / GYRO_SCALE_FACTOR;

  DEBUGS_LOGD("Accel [g] X=%.2f Y=%.2f Z=%.2f", data->accel_x, data->accel_y,
              data->accel_z);
  DEBUGS_LOGD("Gyro  [°/s] X=%.2f Y=%.2f Z=%.2f", data->gyro_x, data->gyro_y,
              data->gyro_z);

  return ESP_OK;
}
