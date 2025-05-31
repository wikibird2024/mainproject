/**
 * @file buzzer.c
 * @brief Driver điều khiển buzzer GPIO cho ESP32 (Active Buzzer)
 * @author Your Name/Team Name
 * @date 31/03/2025
 * @version 2.0
 * @copyright Copyright (c) 2025 Your Company. All rights reserved.
 * @note Chỉ hỗ trợ buzzer chủ động (active buzzer). Đối với buzzer bị động, sử dụng driver PWM riêng.
 */

/* ---------- Includes ---------- */
#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* ---------- Macro Definitions ---------- */
#define BUZZER_GPIO          GPIO_NUM_25   /**< GPIO kết nối buzzer */
#define BUZZER_QUEUE_LENGTH  5             /**< Số lệnh tối đa trong queue */

/* ---------- Static Variables ---------- */
static const char *TAG = "buzzer";         /**< Tag for ESP-IDF logging */
static QueueHandle_t buzzer_queue = NULL;  /**< Queue điều khiển buzzer */

/* ---------- Struct Definitions ---------- */
/**
 * @brief Lệnh điều khiển buzzer
 * @details Gửi qua queue để xử lý bất đồng bộ.
 */
typedef struct {
    int duration_ms;  /**< Thời gian bật (ms). Giá trị âm = bật vô hạn, 0 = tắt ngay */
} buzzer_cmd_t;

/* ---------- Private Functions ---------- */
/**
 * @brief Task nội bộ xử lý bật/tắt buzzer
 * @private
 * @param[in] arg Không sử dụng (NULL)
 */
static void buzzer_task(void *arg) {
    buzzer_cmd_t cmd;
    while (1) {
        if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY)) {
            gpio_set_level(BUZZER_GPIO, 1);  // Bật buzzer
            if (cmd.duration_ms > 0) {
                vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));
                gpio_set_level(BUZZER_GPIO, 0);  // Tắt sau duration_ms
            }
            // Nếu duration_ms < 0: giữ trạng thái bật vô hạn
        }
    }
}

/* ---------- Public Functions ---------- */
/**
 * @defgroup BUZZER_DRIVER Driver Buzzer
 * @brief Nhóm hàm API điều khiển buzzer
 * @{
 */

/**
 * @brief Khởi tạo buzzer
 * @return 
 *      - ESP_OK: Thành công
 *      - ESP_FAIL: Lỗi khởi tạo GPIO hoặc queue
 * @warning Chỉ gọi hàm này một lần duy nhất trong quá trình khởi động hệ thống.
 */
esp_err_t buzzer_init(void) {
    // Cấu hình GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(BUZZER_GPIO, 0);  // Đảm bảo tắt ban đầu

    // Tạo queue (overwrite nếu đầy)
    buzzer_queue = xQueueCreate(BUZZER_QUEUE_LENGTH, sizeof(buzzer_cmd_t));
    if (buzzer_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return ESP_FAIL;
    }

    // Tạo task trên Core 1
    xTaskCreatePinnedToCore(
        buzzer_task,    // Hàm xử lý
        "buzzer_task",  // Tên task
        2048,           // Stack size
        NULL,           // Tham số
        10,             // Priority
        NULL,           // Task handle
        1              // Core 1
    );

    ESP_LOGI(TAG, "Initialized (GPIO %d)", BUZZER_GPIO);
    return ESP_OK;
}

/**
 * @brief Điều khiển buzzer bật/tắt theo thời gian
 * @param[in] duration_ms Thời gian bật (ms). 
 *        - >0: Bật trong khoảng thời gian xác định (ví dụ: 1000 = 1 giây)
 *        - 0: Tắt ngay lập tức
 *        - <0: Bật vô hạn (phải gọi lại với duration_ms=0 để tắt)
 * @return 
 *      - ESP_OK: Gửi lệnh thành công
 *      - ESP_FAIL: Queue lỗi
 * @example 
 * @code
 * buzzer_beep(1000);  // Beep 1 giây
 * buzzer_beep(-1);    // Bật vô hạn
 * buzzer_beep(0);     // Tắt
 * @endcode
 */
esp_err_t buzzer_beep(int duration_ms) {
    if (buzzer_queue == NULL) {
        ESP_LOGE(TAG, "Queue not initialized");
        return ESP_FAIL;
    }

    buzzer_cmd_t cmd = { .duration_ms = duration_ms };
    BaseType_t ret = xQueueSend(buzzer_queue, &cmd, 0);
    
    if (ret != pdPASS) {
        ESP_LOGW(TAG, "Queue full, overwriting...");
        xQueueOverwrite(buzzer_queue, &cmd);  // Ưu tiên lệnh mới nhất
    }

    return ESP_OK;
}

/** @} */ // End of BUZZER_DRIVER group