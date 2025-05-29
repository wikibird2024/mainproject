#include "mpu6050.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

#define TAG "MPU6050"                    // Tag dùng cho ESP_LOG

#define I2C_MASTER_NUM I2C_NUM_0         //Số bus I2C dùng (I2C0)
static const float FALL_THRESHOLD = 0.7f;      // Ngưỡng magnitude gia tốc (g) để phát hiện té ngã
static const float GRAVITY =1.0f;              // Giá trị trọng lực mong đợi 1g
static const float ACCEL_SCALE_FACTOR = 16384.0f;  // Hệ số tỉ lệ cho gia tốc (raw -> g)
static const float GYRO_SCALE_FACTOR = 131.0f;     // Hệ số tỉ lệ cho gyro (raw -> độ/giây)
#define COMBINE_BYTES(msb, lsb) ((int16_t)(((msb) << 8) | (lsb))) // Ghép 2 byte thành int16_t

/* Ghi 1 byte vào thanh ghi của thiết bị I2C */
static esp_err_t i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, write_buf, 2, 100 / portTICK_PERIOD_MS);
}
/* Đọc nhiều byte từ thanh ghi I2C */
static esp_err_t i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, data, len, 100 / portTICK_PERIOD_MS);
}

esp_err_t mpu6050_init(void) {
    ESP_LOGI(TAG, "Initializing MPU6050...");
    // Viết 0x00 vào thanh ghi PWR_MGMT_1 để đánh thức MPU6050 (thoát sleep mode)
    esp_err_t err = i2c_write(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00); // Bỏ chế độ ngủ
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 initialization failed!");
        return err;
    }
    ESP_LOGI(TAG, "MPU6050 initialized.");
    return ESP_OK;
}

esp_err_t mpu6050_read_data(sensor_data_t *data) {
    if (data == NULL) {
        ESP_LOGE(TAG, "Null pointer passed to mpu6050_read_data");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[14];    // 14 byte gồm accel(6), temp(2), gyro(6)
    esp_err_t ret = i2c_read(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, raw, sizeof(raw));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MPU6050 data!");
        return ret;
    }

    data->accel_x = COMBINE_BYTES(raw[0], raw[1]) / ACCEL_SCALE_FACTOR;
    data->accel_y = COMBINE_BYTES(raw[2], raw[3]) / ACCEL_SCALE_FACTOR;
    data->accel_z = COMBINE_BYTES(raw[4], raw[5]) / ACCEL_SCALE_FACTOR;
    data->gyro_x  = COMBINE_BYTES(raw[8], raw[9]) / GYRO_SCALE_FACTOR;
    data->gyro_y  = COMBINE_BYTES(raw[10], raw[11]) / GYRO_SCALE_FACTOR;
    data->gyro_z  = COMBINE_BYTES(raw[12], raw[13]) / GYRO_SCALE_FACTOR;

    ESP_LOGI(TAG, "Accel X: %.2f, Y: %.2f, Z: %.2f", data->accel_x, data->accel_y, data->accel_z);
    ESP_LOGI(TAG, "Gyro  X: %.2f, Y: %.2f, Z: %.2f", data->gyro_x, data->gyro_y, data->gyro_z);

    return ESP_OK;
}

bool detect_fall(sensor_data_t data) {
    // Tính độ lớn vector gia tốc (bỏ qua trọng lực)
    float acc_magnitude = sqrt(
        data.accel_x * data.accel_x +
        data.accel_y * data.accel_y +
        data.accel_z * data.accel_z
    );
    
     float acc_delta = fabs(acc_magnitude - 1.0f); // Tính độ lớn vector gia tốc, rồi lấy sai lệch so với trọng lực 1g


    ESP_LOGI(TAG, "Accel magnitude: %.2f g, acc_delta: %.2f g", acc_magnitude, acc_delta);

    if (acc_delta > FALL_THRESHOLD) {
        ESP_LOGW(TAG, "Fall detected! (delta=%.2f g)", acc_delta);
        return true;
    }
    return false;
}
