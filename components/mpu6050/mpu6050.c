#include "mpu6050.h"
#include "comm.h"
#include "debugs.h"
#include <math.h>
#include <string.h>

#define MPU6050_ADDR             0x68
#define MPU6050_PWR_MGMT_1       0x6B
#define MPU6050_ACCEL_XOUT_H     0x3B

#define COMBINE_BYTES(msb, lsb) ((int16_t)(((msb) << 8) | (lsb)))

static const float ACCEL_SCALE_FACTOR = 16384.0f;   // raw to g
static const float GYRO_SCALE_FACTOR  = 131.0f;     // raw to deg/s
static const float FALL_THRESHOLD     = 0.7f;       // g difference

esp_err_t mpu6050_init(void)
{
    DEBUGS_LOGI("Initializing MPU6050...");

    esp_err_t err = comm_i2c_write(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
    if (err != ESP_OK) {
        DEBUGS_LOGE("MPU6050 I2C init failed: %s", esp_err_to_name(err));
        return err;
    }

    DEBUGS_LOGI("MPU6050 initialized successfully.");
    return ESP_OK;
}

esp_err_t mpu6050_read_data(sensor_data_t *data)
{
    if (data == NULL) {
        DEBUGS_LOGE("Null pointer passed to mpu6050_read_data");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[14] = {0};
    esp_err_t err = comm_i2c_read(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, raw, sizeof(raw));
    if (err != ESP_OK) {
        DEBUGS_LOGE("MPU6050 I2C read failed: %s", esp_err_to_name(err));
        return err;
    }

    data->accel_x = COMBINE_BYTES(raw[0], raw[1]) / ACCEL_SCALE_FACTOR;
    data->accel_y = COMBINE_BYTES(raw[2], raw[3]) / ACCEL_SCALE_FACTOR;
    data->accel_z = COMBINE_BYTES(raw[4], raw[5]) / ACCEL_SCALE_FACTOR;

    data->gyro_x  = COMBINE_BYTES(raw[8], raw[9])   / GYRO_SCALE_FACTOR;
    data->gyro_y  = COMBINE_BYTES(raw[10], raw[11]) / GYRO_SCALE_FACTOR;
    data->gyro_z  = COMBINE_BYTES(raw[12], raw[13]) / GYRO_SCALE_FACTOR;

    DEBUGS_LOGD("Accel [g] X=%.2f Y=%.2f Z=%.2f", data->accel_x, data->accel_y, data->accel_z);
    DEBUGS_LOGD("Gyro  [°/s] X=%.2f Y=%.2f Z=%.2f", data->gyro_x, data->gyro_y, data->gyro_z);

    return ESP_OK;
}

bool mpu6050_detect_fall(const sensor_data_t *data)
{
    if (data == NULL) {
        DEBUGS_LOGW("Null pointer passed to mpu6050_detect_fall");
        return false;
    }

    float acc_magnitude = sqrtf(
        data->accel_x * data->accel_x +
        data->accel_y * data->accel_y +
        data->accel_z * data->accel_z
    );

    float delta = fabsf(acc_magnitude - 1.0f);

    DEBUGS_LOGI("Accel magnitude: %.2f g | Delta: %.2f g", acc_magnitude, delta);

    if (delta > FALL_THRESHOLD) {
        DEBUGS_LOGW("Fall detected! Δ=%.2f g", delta);
        return true;
    }

    return false;
}
