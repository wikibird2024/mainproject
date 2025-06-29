#include "mpu6050.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

/// ============================== Configuration ==============================

#define TAG "MPU6050"
#define I2C_MASTER_PORT I2C_NUM_0
#define I2C_TIMEOUT_MS 100

#define COMBINE_BYTES(msb, lsb) ((int16_t)(((msb) << 8) | (lsb)))

static const float ACCEL_SCALE = 16384.0f;   // Raw to g
static const float GYRO_SCALE = 131.0f;      // Raw to deg/sec
static const float FALL_THRESHOLD = 0.7f;    // Threshold to detect fall (g)


/// ============================== Static Functions ==============================

/**
 * @brief Write a byte to a register over I2C.
 */
static esp_err_t mpu6050_write_byte(uint8_t device_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t buffer[2] = {reg_addr, data};
    return i2c_master_write_to_device(
        I2C_MASTER_PORT,device_addr,buffer,sizeof(buffer),I2C_TIMEOUT_MS / portTICK_PERIOD_MS
        );
}

/**
 * @brief Read multiple bytes from MPU6050 via I2C.
 */
static esp_err_t mpu6050_read_bytes(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(
        I2C_MASTER_PORT, device_addr, &reg_addr, 1, data, len, I2C_TIMEOUT_MS / portTICK_PERIOD_MS
    );
}


/// ============================== Public Functions ==============================

/**
 * @brief Initialize MPU6050 by waking it up from sleep mode.
 */
esp_err_t mpu6050_init(void)
{
    ESP_LOGI(TAG, "Initializing MPU6050...");

    esp_err_t err = mpu6050_write_byte(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MPU6050 (PWR_MGMT_1 write failed)");
        return err;
    }

    ESP_LOGI(TAG, "MPU6050 initialized successfully.");
    return ESP_OK;
}

/**
 * @brief Read raw accelerometer and gyroscope data and convert to physical units.
 */
esp_err_t mpu6050_read_data(sensor_data_t *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Null pointer provided to mpu6050_read_data");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[14];
    esp_err_t err = mpu6050_read_bytes(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, raw, sizeof(raw));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data from MPU6050");
        return err;
    }

    data->accel_x = COMBINE_BYTES(raw[0], raw[1]) / ACCEL_SCALE;
    data->accel_y = COMBINE_BYTES(raw[2], raw[3]) / ACCEL_SCALE;
    data->accel_z = COMBINE_BYTES(raw[4], raw[5]) / ACCEL_SCALE;

    data->gyro_x  = COMBINE_BYTES(raw[8], raw[9])   / GYRO_SCALE;
    data->gyro_y  = COMBINE_BYTES(raw[10], raw[11]) / GYRO_SCALE;
    data->gyro_z  = COMBINE_BYTES(raw[12], raw[13]) / GYRO_SCALE;

    ESP_LOGI(TAG, "Accel: X=%.2fg Y=%.2fg Z=%.2fg", data->accel_x, data->accel_y, data->accel_z);
    ESP_LOGI(TAG, "Gyro : X=%.2f°/s Y=%.2f°/s Z=%.2f°/s", data->gyro_x, data->gyro_y, data->gyro_z);

    return ESP_OK;
}

/**
 * @brief Detect fall based on deviation of total acceleration from 1g.
 */
bool mpu6050_detect_fall(const sensor_data_t *data)
{
    if (data == NULL) {
        ESP_LOGW(TAG, "Null pointer passed to mpu6050_detect_fall");
        return false;
    }

    float acc_magnitude = sqrtf(
        data->accel_x * data->accel_x +
        data->accel_y * data->accel_y +
        data->accel_z * data->accel_z
    );

    float delta = fabsf(acc_magnitude - 1.0f);

    ESP_LOGI(TAG, "Accel magnitude: %.2fg, Δ: %.2fg", acc_magnitude, delta);

    if (delta > FALL_THRESHOLD) {
        ESP_LOGW(TAG, "Fall detected! Δ=%.2fg", delta);
        return true;
    }

    return false;
}
