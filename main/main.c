#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "mpu6050.h"
#include "sim4g_gps.h"
#include "buzzer.h"
#include "comm.h"
#include "debugs.h"
#include "led_indicator.h"
// Đinh nghia chan ket noi
#define LED_GPIO    2           // GPIO dieu khien led canh bao
#define BUTTON_GPIO -1          // Khong su dung nut nhan

// Thoi gian kiem tra va canh bao
#define CHECK_INTERVAL_MS 1000  // kiem tra te nga moi 1000ms
#define ALERT_DURATION_MS 5000  // Thoi gian canh bao


#define PHONE_NUMBER "+84559865843"

// Semaphore 
static SemaphoreHandle_t xSemaphore = NULL;

// Khoi tao toan bo he thong
void system_init(void) {
    INFO("Khởi tạo hệ thống...");

    debugs_init();  // Cau hinhlog toan bo he thong

    comm_uart_init();               // UART giao tiep module SIM
    sim4g_gps_init();              // bat GPS module SIM
    sim4g_gps_set_phone_number(PHONE_NUMBER);  // SĐT nhận cảnh báo

    comm_i2c_init();               // I2C giao tiep MPU6050
    if (mpu6050_init() != ESP_OK) {
        ERROR("Lỗi khởi tạo MPU6050");
    } else {
        INFO("MPU6050 đã sẵn sàng");
    }

    buzzer_init();                 // khoi tao buzzer (GPIO hoặc PWM)
    comm_gpio_init(LED_GPIO, BUTTON_GPIO);  // khoi tao LED
    comm_gpio_led_set(0);         // Tat led ban dau

    // Tao semaphore dong bo tai nguyen
    xSemaphore = xSemaphoreCreateMutex();
    if (xSemaphore == NULL) {
        ERROR("Không thể tạo semaphore");
    }
}

// Task kiem tra te nga dinh ky
void fall_detection_task(void *param) {
    INFO("Bắt đầu task phát hiện té ngã");
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
            sensor_data_t data = mpu6050_read_data();  // Doc du lieu mpu6050

            DEBUG("Gia tốc: X=%.2f Y=%.2f Z=%.2f | Gyro: X=%.2f Y=%.2f Z=%.2f",
                  data.accel_x, data.accel_y, data.accel_z,
                  data.gyro_x, data.gyro_y, data.gyro_z);

            if (detect_fall(data)) {
                INFO("Phát hiện té ngã! Gửi cảnh báo...");

                gps_data_t location = sim4g_gps_get_location();  // Lay vi tri GPS

                // Gui SMS canh bao ke ca khi khong lay duoc GPS
                if (!location.valid) {
                    WARN("Không thể lấy vị trí GPS. Gửi SMS không có vị trí.");
                }

                // Gui sms canh bao (co hoac khong co link GPS)
                send_fall_alert_sms(&location);  

                buzzer_beep(ALERT_DURATION_MS);  // Bat coi 
                comm_gpio_led_set(1);            // Bat led canh bao
                vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));  // Giu LED sang
                comm_gpio_led_set(0);            // Tat led
            }

            xSemaphoreGive(xSemaphore);  
        }

        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));  // Kiem tra lai sau mot khoan thoi gian
    }
}

// Ham main

void app_main(void) {
    system_init();  // Thiet lap he thong
    xTaskCreate(fall_detection_task, "fall_task", 4096, NULL, 5, NULL);  // Tao task kiem tra te nga
}
