#include "mpu6050.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define TAG "MPU6050"
#define I2C_MASTER_NUM I2C_NUM_0

static esp_err_t i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, write_buf, 2, 100 / portTICK_PERIOD_MS);
}

static esp_err_t i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, data, len, 100 / portTICK_PERIOD_MS);
}

esp_err_t mpu6050_init(void) {
    ESP_LOGI(TAG, "Initializing MPU6050...");
    esp_err_t err = i2c_write(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00); // Bỏ chế độ ngủ
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 initialization failed!");
        return err;
    }
    ESP_LOGI(TAG, "MPU6050 initialized.");
    return ESP_OK;
}

sensor_data_t mpu6050_read_data(void) {
    uint8_t raw_data[14];
    sensor_data_t data = {0};

    if (i2c_read(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, raw_data, 14) == ESP_OK) {
        data.accel_x = (int16_t)((raw_data[0] << 8) | raw_data[1]) / 16384.0;
        data.accel_y = (int16_t)((raw_data[2] << 8) | raw_data[3]) / 16384.0;
        data.accel_z = (int16_t)((raw_data[4] << 8) | raw_data[5]) / 16384.0;
        data.gyro_x  = (int16_t)((raw_data[8] << 8) | raw_data[9]) / 131.0;
        data.gyro_y  = (int16_t)((raw_data[10] << 8) | raw_data[11]) / 131.0;
        data.gyro_z  = (int16_t)((raw_data[12] << 8) | raw_data[13]) / 131.0;
        // prin data log
        ESP_LOGI(TAG, "Accel X: %.2f, Accel Y: %.2f, Accel Z: %.2f", data.accel_x, data.accel_y, data.accel_z);
        ESP_LOGI(TAG, "Gyro X: %.2f, Gyro Y: %.2f, Gyro Z: %.2f", data.gyro_x, data.gyro_y, data.gyro_z);
    } else {
        ESP_LOGE(TAG, "Failed to read MPU6050 data!");
    }

    return data;
}

bool detect_fall(sensor_data_t data) {
    float acc_magnitude = data.accel_x * data.accel_x +
                          data.accel_y * data.accel_y +
                          data.accel_z * data.accel_z;
                              // In giá trị magnitude vào log để theo dõi
    ESP_LOGI(TAG, "Accelerometer magnitude: %.2f", acc_magnitude);
    
    // Ngưỡng phát hiện té ngã (giá trị thử nghiệm)
    if (acc_magnitude < 0.3) {
        ESP_LOGW(TAG, "Fall detected! Acceleration magnitude: %.2f", acc_magnitude);
        return true;
    }

    return false;
}
