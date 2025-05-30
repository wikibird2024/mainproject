/**
 * @file main.c
 * @brief Chương trình chính cho hệ thống cảnh báo té ngã sử dụng ESP32, MPU6050, SIM4G-GPS.
 *
 * Các chức năng chính:
 *  - Đọc dữ liệu cảm biến MPU6050 để phát hiện té ngã.
 *  - Khi phát hiện té ngã, gửi sự kiện cảnh báo.
 *  - Xử lý cảnh báo: phát còi, nháy LED, gửi tin nhắn SMS kèm vị trí GPS.
 */

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

<<<<<<< HEAD
// --- Định nghĩa các chân GPIO và hằng số cấu hình ---

#define LED_GPIO           2           /**< Chân GPIO kết nối LED chỉ thị */
#define BUTTON_GPIO        -1          /**< Chân GPIO nút nhấn (không dùng) */
#define CHECK_INTERVAL_MS  1000        /**< Chu kỳ kiểm tra cảm biến (ms) */
#define ALERT_DURATION_MS  8000        /**< Thời gian phát cảnh báo (ms) */
#define LED_BLINK_PERIOD   100         /**< Chu kỳ nháy LED khi cảnh báo (ms) */

static const char *PHONE_NUMBER = "+84559865843"; /**< Số điện thoại nhận SMS cảnh báo */

typedef enum {
    FALL_DETECTED  /**< Sự kiện phát hiện té ngã */
} system_event_t;


// --- Biến toàn cục quản lý queue và mutex ---

static QueueHandle_t xEventQueue = NULL;       /**< Queue nhận sự kiện hệ thống */
static SemaphoreHandle_t xMutex = NULL;        /**< Mutex bảo vệ truy cập cảm biến */


// --- Hàm khởi tạo hệ thống ---

/**
 * @brief Khởi tạo các thành phần hệ thống: mutex, queue, các module phần cứng và phần mềm.
 * 
 * Hàm sẽ khởi tạo và kiểm tra thành công các tài nguyên quan trọng,
 * nếu có lỗi sẽ reset ESP để tránh hoạt động không ổn định.
 */
void system_init(void)
{
    INFO("Khởi tạo hệ thống...");

    // Tạo mutex với số lần thử tối đa
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

    // Tạo queue sự kiện
    xEventQueue = xQueueCreate(5, sizeof(system_event_t));
    if (xEventQueue == NULL) {
        ERROR("Không thể tạo event queue");
        esp_restart();
    }

    // Khởi tạo các module con
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

    buzzer_init();

    led_indicator_config_t led_config = {
        .gpio_num = LED_GPIO,
        .active_high = true
    };

    if (led_indicator_init(&led_config) != ESP_OK) {
        ERROR("Khởi tạo LED thất bại");
    }
}


// --- Task phát hiện té ngã ---

/**
 * @brief Task đọc dữ liệu từ MPU6050 để phát hiện té ngã.
 *
 * Khi phát hiện té ngã, gửi sự kiện tới queue để xử lý cảnh báo.
 * Dữ liệu cảm biến được bảo vệ bằng mutex.
 */
void fall_detection_task(void *param)
{
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
                    xQueueSend(xEventQueue, &event, 0);
                }
            } else {
                ERROR("Không đọc được dữ liệu từ MPU6050");
            }
            xSemaphoreGive(xMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
    }
}


// --- Task xử lý cảnh báo ---

/**
 * @brief Task nhận sự kiện cảnh báo từ queue và xử lý.
 *
 * Khi nhận được sự kiện FALL_DETECTED, task sẽ:
 *  - Lấy vị trí GPS.
 *  - Gửi tin nhắn cảnh báo với vị trí (nếu có).
 *  - Phát còi và nháy LED trong thời gian định sẵn.
 */
void alert_task(void *param)
{
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

                sim4g_gps_send_fall_alert_async(&location);

                buzzer_beep(ALERT_DURATION_MS);
                led_indicator_start_blink(LED_BLINK_PERIOD, LED_BLINK_DOUBLE);
                vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));
                led_indicator_stop_blink();
            }
        }
    }
}


// --- Hàm chính ---

/**
 * @brief Hàm main của ứng dụng.
 *
 * Khởi tạo hệ thống và tạo các task xử lý.
 */
void app_main(void)
{
    system_init();

    if (xTaskCreate(fall_detection_task, "fall_task", 4096, NULL, 5, NULL) != pdPASS) {
        ERROR("Không thể tạo task fall_detection_task");
    }

    if (xTaskCreate(alert_task, "alert_task", 4096, NULL, 5, NULL) != pdPASS) {
        ERROR("Không thể tạo task alert_task");
    }
}
