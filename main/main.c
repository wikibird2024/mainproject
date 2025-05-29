#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "mpu6050.h"
#include "sim4g_gps.h"
#include "buzzer.h"
#include "comm.h"
#include "debugs.h"
#include "led_indicator.h"


// Định nghĩa chân kết nối
#define LED_GPIO    2           // GPIO điều khiển led cảnh báo
#define BUTTON_GPIO -1          // Không sử dụng nút nhấn

// Thời gian kiểm tra và cảnh báo
#define CHECK_INTERVAL_MS 1000  // kiểm tra té ngã mỗi 1000ms
#define ALERT_DURATION_MS 5000  // Thời gian cảnh báo

static const char *PHONE_NUMBER = "+84559865843";


// Semaphore (mutex) để đồng bộ tài nguyên chung
static SemaphoreHandle_t xMutex = NULL;

// Khởi tạo toàn bộ hệ thống
void system_init(void) {
    INFO("Khởi tạo hệ thống...");
    
 // --- Tạo mutex với cơ chế retry ---
    const int max_retry = 3;
    for (int i = 0; i < max_retry; i++) {
        xMutex = xSemaphoreCreateMutex();
        if (xMutex != NULL) break;
        WARN("Tạo semaphore thất bại, thử lại (%d/%d)", i + 1, max_retry);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (xMutex == NULL) {
        ERROR("Không thể tạo semaphore sau %d lần thử. Khởi động lại hệ thống.", max_retry);
        vTaskDelay(pdMS_TO_TICKS(500));  // chờ để log được in ra hết
        esp_restart();  // reset thiết bị
    }
//---cac phan khoi tao he thog--
    debugs_init();              // Cấu hình log toàn bộ hệ thống
    comm_uart_init();           // UART giao tiếp module SIM
    sim4g_gps_init();           // Bật GPS module SIM
    sim4g_gps_set_phone_number(PHONE_NUMBER);  // SĐT nhận cảnh báo

    comm_i2c_init();            // I2C giao tiếp MPU6050
    if (mpu6050_init() != ESP_OK) {
        ERROR("Lỗi khởi tạo MPU6050");
    } else {
        INFO("MPU6050 đã sẵn sàng");
    }

    buzzer_init();              // Khởi tạo buzzer (GPIO hoặc PWM)
     // Khởi tạo led_indicator component, truyền chân GPIO led
    led_indicator_init(LED_GPIO);
}
// Task kiểm tra té ngã định kỳ
void fall_detection_task(void *param) {
    INFO("Bắt đầu task phát hiện té ngã");
    while (1) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
            sensor_data_t data;

            // Đọc dữ liệu MPU6050, kiểm tra lỗi
            if (mpu6050_read_data(&data) == ESP_OK) {
                DEBUG("Gia tốc: X=%.2f Y=%.2f Z=%.2f | Gyro: X=%.2f Y=%.2f Z=%.2f",
                      data.accel_x, data.accel_y, data.accel_z,
                      data.gyro_x, data.gyro_y, data.gyro_z);

                if (detect_fall(data)) {
                    INFO("Phát hiện té ngã! Gửi cảnh báo...");

                    gps_data_t location = sim4g_gps_get_location();

                    if (!location.valid) {
                        WARN("Không thể lấy vị trí GPS. Gửi SMS không có vị trí.");
                    }

                    send_fall_alert_sms(&location);

                    buzzer_beep(ALERT_DURATION_MS);
                    // Dùng hàm nháy led của led_indicator
                    led_indicator_blink(ALERT_DURATION_MS);

                    // Nếu  không muốn nháy, mà chỉ bật rồi tắt:
                    // led_indicator_on();
                    // vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));
                    // led_indicator_off();
                }
            } else {
                ERROR("Không đọc được dữ liệu từ MPU6050");
            }

            xSemaphoreGive(xMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
    }
}

// Hàm main
void app_main(void) {
    system_init();  // Thiết lập hệ thống

    BaseType_t task_created = xTaskCreate(fall_detection_task, "fall_task", 4096, NULL, 5, NULL);
    if (task_created != pdPASS) {
        ERROR("Không thể tạo task fall_detection_task");
    }
}
