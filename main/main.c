#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mpu6050.h"
#include "sim4g_gps.h"
#include "buzzer.h"
#include "comm.h"
#include "debugs.h"

// Định nghĩa chân kết nối phần cứng
#define LED_GPIO    2           // GPIO điều khiển LED cảnh báo
#define BUTTON_GPIO -1          // Không sử dụng nút nhấn

// Thời gian kiểm tra và cảnh báo
#define CHECK_INTERVAL_MS 1000  // Kiểm tra té ngã mỗi 1000ms
#define ALERT_DURATION_MS 5000  // Cảnh báo kéo dài 5 giây

// Số điện thoại nhận cảnh báo
#define PHONE_NUMBER "+84123456789"

// Khởi tạo toàn bộ hệ thống
void system_init(void) {
    INFO("Khởi tạo hệ thống...");

    debugs_init();  // Cấu hình log toàn hệ thống

    comm_uart_init();               // UART giao tiếp module SIM
    sim4g_gps_init();              // Bật GPS trên module SIM
    sim4g_gps_set_phone_number(PHONE_NUMBER);  // SĐT nhận cảnh báo

    comm_i2c_init();               // I2C giao tiếp cảm biến MPU6050
    if (mpu6050_init() != ESP_OK) {
        ERROR("Lỗi khởi tạo MPU6050");
    } else {
        INFO("MPU6050 đã sẵn sàng");
    }

    buzzer_init();                 // Khởi tạo buzzer (GPIO hoặc PWM)
    comm_gpio_init(LED_GPIO, BUTTON_GPIO);  // Khởi tạo LED
    comm_gpio_led_set(0);         // Tắt LED lúc đầu
}

// Task kiểm tra té ngã định kỳ
void fall_detection_task(void *param) {
    while (1) {
        sensor_data_t data = mpu6050_read_data();  // Đọc dữ liệu cảm biến

        DEBUG("Gia tốc: X=%.2f Y=%.2f Z=%.2f | Gyro: X=%.2f Y=%.2f Z=%.2f",
              data.accel_x, data.accel_y, data.accel_z,
              data.gyro_x, data.gyro_y, data.gyro_z);

        if (detect_fall(data)) {  // Kiểm tra có té ngã không
            INFO("Phát hiện té ngã! Gửi cảnh báo...");
            gps_data_t location = sim4g_gps_get_location(); // Lấy vị trí GPS
            if (!location.valid) {
                WARN("Không thể lấy vị trí GPS. SMS sẽ không được gửi.");
            }
            send_fall_alert_sms(location);                  // Gửi SMS cảnh báo

            buzzer_beep(ALERT_DURATION_MS);  // Bật còi không blocking
            comm_gpio_led_set(1);            // Bật LED cảnh báo
            vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));  // Giữ LED sáng
            comm_gpio_led_set(0);            // Tắt LED
        }

        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));  // Chờ trước lần kiểm tra tiếp theo
    }
}

// Hàm main chạy đầu tiên
void app_main(void) {
    system_init();  // Thiết lập hệ thống
    xTaskCreate(fall_detection_task, "fall_task", 4096, NULL, 5, NULL);  // Tạo task kiểm tra té ngã
}
