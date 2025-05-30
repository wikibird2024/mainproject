#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "mpu6050.h"
#include "sim4g_gps.h"
#include "buzzer.h"
#include "comm.h"
#include "debugs.h"
#include "led_indicator.h"

// Định nghĩa chân kết nối
#define LED_GPIO           2
#define BUTTON_GPIO        -1  // Không dùng
#define CHECK_INTERVAL_MS  1000
#define ALERT_DURATION_MS  8000
#define LED_BLINK_PERIOD   100
static const char *PHONE_NUMBER = "+84559865843";

// Định nghĩa queue event
typedef enum {
    FALL_DETECTED
} system_event_t;

// Queue và Mutex
static QueueHandle_t xEventQueue = NULL;
static SemaphoreHandle_t xMutex = NULL;

// Khởi tạo toàn bộ hệ thống
void system_init(void) {
    INFO("Khởi tạo hệ thống...");

    // Tạo mutex
    const int max_retry = 3;
    for (int i = 0; i < max_retry; i++) {
        xMutex = xSemaphoreCreateMutex();
        if (xMutex != NULL) break;
        WARN("Tạo semaphore thất bại, thử lại (%d/%d)", i + 1, max_retry);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (xMutex == NULL) {
        ERROR("Không thể tạo semaphore sau %d lần thử. Khởi động lại hệ thống.", max_retry);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }

    // Tạo queue
    xEventQueue = xQueueCreate(5, sizeof(system_event_t));
    if (xEventQueue == NULL) {
        ERROR("Không thể tạo event queue");
        esp_restart();
    }

    debugs_init();
    comm_uart_init();
    sim4g_gps_init();
    sim4g_gps_set_phone_number(PHONE_NUMBER);

    comm_i2c_init();
    if (mpu6050_init() != ESP_OK) {
        ERROR("Lỗi khởi tạo MPU6050");
    } else {
        INFO("MPU6050 đã sẵn sàng");
    }
    // khởi tạo buzzer
    buzzer_init();

    // khởi tạo Led
    led_indicator_config_t led_config = {
        .gpio_num = LED_GPIO,
        .active_high = true  // LED sáng khi GPIO HIGH
    };

    if (led_indicator_init(&led_config) != ESP_OK) {
        ERROR("Khởi tạo LED thất bại");
    }
}

// Task 1: Kiểm tra té ngã
void fall_detection_task(void *param) {
    INFO("Bắt đầu task phát hiện té ngã");
    while (1) {
        sensor_data_t data;

        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(1000))) {
            if (mpu6050_read_data(&data) == ESP_OK) {
                DEBUG("Gia tốc: X=%.2f Y=%.2f Z=%.2f | Gyro: X=%.2f Y=%.2f Z=%.2f",
                      data.accel_x, data.accel_y, data.accel_z,
                      data.gyro_x, data.gyro_y, data.gyro_z);

                if (detect_fall(data)) {
                    INFO("Phát hiện té ngã, gửi sự kiện tới alert_task");
                    system_event_t event = FALL_DETECTED;
                    xQueueSend(xEventQueue, &event, 0);  // gửi không chờ
                }
            } else {
                ERROR("Không đọc được dữ liệu từ MPU6050");
            }
            xSemaphoreGive(xMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
    }
}

// Task 2: Xử lý cảnh báo
void alert_task(void *param) {
    INFO("Bắt đầu task cảnh báo");
    system_event_t event;
    while (1) {
        if (xQueueReceive(xEventQueue, &event, portMAX_DELAY)) {
            if (event == FALL_DETECTED) {
                INFO("Xử lý cảnh báo té ngã...");
                sim4g_gps_data_t location = sim4g_gps_get_location();

                if (!location.valid) {
                    WARN("Không thể lấy vị trí GPS. Gửi SMS không có vị trí.");
                }

                send_fall_alert_sms(&location);

                buzzer_beep(ALERT_DURATION_MS);
                led_indicator_start_blink(LED_BLINK_PERIOD, LED_BLINK_DOUBLE);
                vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));
                led_indicator_stop_blink();
            }
        }
    }
}

// Hàm main
void app_main(void) {
    system_init();

    if (xTaskCreate(fall_detection_task, "fall_task", 4096, NULL, 5, NULL) != pdPASS) {
        ERROR("Không thể tạo task fall_detection_task");
    }

    if (xTaskCreate(alert_task, "alert_task", 4096, NULL, 5, NULL) != pdPASS) {
        ERROR("Không thể tạo task alert_task");
    }
}
