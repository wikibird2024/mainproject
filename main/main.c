/**
 * @file main.c
 * @brief Hệ thống phát hiện té ngã sử dụng ESP32, MPU6050 và module 4G-GPS EC800K.
 *
 * @details Ứng dụng này phát hiện té ngã thông qua dữ liệu từ cảm biến MPU6050,
 * gửi tọa độ GPS qua SMS bằng module Quectel EC800K,
 * và phát cảnh báo cục bộ bằng còi và đèn LED.
 *
 * @date
 *  Cập nhật: 2025-06-1
 */

// --- Hằng số cấu hình ---
#define LED_GPIO             2                   /**< Chân GPIO dùng cho đèn LED cảnh báo */
#define CHECK_INTERVAL_MS    1000                /**< Thời gian kiểm tra té ngã (ms) */
#define ALERT_DURATION_MS    8000                /**< Thời gian cảnh báo sau khi té ngã (ms) */
#define LED_BLINK_PERIOD     100                 /**< Chu kỳ nhấp nháy của LED (ms) */
#define MAX_RETRY            3                   /**< Số lần thử tối đa khi tạo tài nguyên */
#define MUTEX_TIMEOUT_MS     1000                /**< Thời gian chờ mutex (ms) */

static const char *PHONE_NUMBER = "+84559865843"; /**< Số điện thoại khẩn cấp được cấu hình sẵn */

// --- Liệt kê các sự kiện hệ thống ---
/**
 * @brief Các sự kiện hệ thống sử dụng trong hàng đợi sự kiện.
 */
typedef enum {
    FALL_DETECTED     /**< Sự kiện được kích hoạt khi phát hiện té ngã */
} system_event_t;

// --- Các đối tượng toàn cục ---
static QueueHandle_t xEventQueue = NULL;  /**< Hàng đợi sự kiện hệ thống */
static SemaphoreHandle_t xMutex = NULL;   /**< Mutex để chia sẻ quyền truy cập I2C */

/**
 * @brief Hàm callback xử lý kết quả gửi SMS.
 *
 * @param success true nếu gửi SMS thành công, false nếu thất bại.
 */
void sms_callback(bool success) {
    if (success) {
        INFO("Gửi SMS thành công");
    } else {
        ERROR("Gửi SMS thất bại");
    }
}

/**
 * @brief Khởi tạo các thành phần và tài nguyên hệ thống.
 *
 * @note Hàm này phải được gọi trước khi tạo các task.
 *       Nếu thất bại, hệ thống sẽ tự khởi động lại.
 */
void system_init(void) {
    INFO("Khởi tạo hệ thống...");

    // Tạo mutex với cơ chế thử lại
    for (int i = 0; i < MAX_RETRY; i++) {
        xMutex = xSemaphoreCreateMutex();
        if (xMutex) break;
        WARN("Tạo mutex thất bại (%d/%d)", i + 1, MAX_RETRY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!xMutex) {
        ERROR("Không tạo được mutex sau %d lần", MAX_RETRY);
        esp_restart();
    }

    // Tạo hàng đợi sự kiện
    xEventQueue = xQueueCreate(5, sizeof(system_event_t));
    if (!xEventQueue) {
        ERROR("Tạo queue thất bại");
        esp_restart();
    }

    // Khởi tạo các hệ thống con
    debugs_init();
    comm_uart_init();
    sim4g_gps_init();
    if (sim4g_gps_set_phone_number(PHONE_NUMBER) != SIM4G_SUCCESS) {
        ERROR("Thiết lập số điện thoại thất bại");
        esp_restart();
    }

    comm_i2c_init();
    if (mpu6050_init() != ESP_OK) {
        ERROR("Lỗi khởi tạo MPU6050");
        esp_restart();
    }

    buzzer_init();
    led_indicator_config_t led_config = {
        .gpio_num = LED_GPIO,
        .active_high = true
    };
    if (led_indicator_init(&led_config) != ESP_OK) {
        ERROR("Khởi tạo LED thất bại");
        esp_restart();
    }

    INFO("Khởi tạo hoàn tất");
}

/**
 * @brief Task liên tục đọc dữ liệu cảm biến và phát hiện té ngã.
 *
 * @param[in] param Không sử dụng (truyền NULL).
 *
 * @note Sử dụng mutex để truy cập an toàn cảm biến MPU6050.
 */
void fall_detection_task(void *param) {
    INFO("Task phát hiện té ngã bắt đầu");

    while (1) {
        sensor_data_t data;

        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS))) {
            if (mpu6050_read_data(&data) == ESP_OK) {
                if (detect_fall(data)) {
                    INFO("Té ngã: Accel=(%.2f,%.2f,%.2f), Gyro=(%.2f,%.2f,%.2f)",
                         data.accel_x, data.accel_y, data.accel_z,
                         data.gyro_x, data.gyro_y, data.gyro_z);

                    system_event_t event = FALL_DETECTED;
                    if (xQueueSend(xEventQueue, &event, 0) != pdTRUE) {
                        WARN("Queue đầy, bỏ qua sự kiện té ngã");
                    }
                }
            } else {
                ERROR("Đọc MPU6050 thất bại");
            }

            xSemaphoreGive(xMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));
    }
}

/**
 * @brief Task xử lý cảnh báo hệ thống (SMS, còi, LED) khi có sự kiện.
 *
 * @param[in] param Không sử dụng (truyền NULL).
 */
void alert_task(void *param) {
    INFO("Task xử lý cảnh báo bắt đầu");

    system_event_t event;

    while (1) {
        if (xQueueReceive(xEventQueue, &event, portMAX_DELAY) == pdTRUE) {
            if (event == FALL_DETECTED) {
                INFO("Xử lý sự kiện té ngã...");

                sim4g_gps_data_t location = sim4g_gps_get_location();
                if (!location.valid) {
                    WARN("Không lấy được vị trí GPS");
                }

                sim4g_gps_send_fall_alert_async(&location, sms_callback);

                buzzer_beep(ALERT_DURATION_MS);
                led_indicator_start_blink(LED_BLINK_PERIOD, LED_BLINK_DOUBLE);

                vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));

                buzzer_stop();
                led_indicator_stop_blink();
            }
        }
    }
}

/**
 * @brief Hàm main của ứng dụng. Khởi tạo hệ thống và khởi chạy các task.
 *
 * @note Nếu tạo task thất bại, hệ thống sẽ tự khởi động lại.
 */
void app_main(void) {
    system_init();

    BaseType_t fall_ok = xTaskCreate(fall_detection_task, "fall_task", 4096, NULL, 5, NULL);
    BaseType_t alert_ok = xTaskCreate(alert_task, "alert_task", 4096, NULL, 5, NULL);

    if (fall_ok != pdPASS || alert_ok != pdPASS) {
        ERROR("Tạo task thất bại");
        vSemaphoreDelete(xMutex);
        vQueueDelete(xEventQueue);
        esp_restart();
    }
}