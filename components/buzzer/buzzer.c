#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define BUZZER_GPIO GPIO_NUM_25 /**< GPIO chân kết nối buzzer */

/**
 * @brief Tag dùng cho ESP_LOG
 */
static const char *TAG = "buzzer";

/**
 * @brief Handle queue nhận lệnh điều khiển buzzer
 */
static QueueHandle_t buzzer_queue = NULL;

/**
 * @brief Cấu trúc lệnh bật buzzer
 */
typedef struct {
    int duration_ms; /**< Thời gian buzzer bật, tính bằng milliseconds */
} buzzer_cmd_t;

/**
 * @brief Task xử lý bật/tắt buzzer dựa trên lệnh nhận từ queue
 *
 * Task này sẽ liên tục chờ nhận lệnh bật buzzer qua queue.
 * Khi nhận lệnh, task sẽ bật buzzer trong thời gian yêu cầu,
 * sau đó tắt buzzer.
 *
 * @param arg Không sử dụng
 */
static void buzzer_task(void *arg)
{
    buzzer_cmd_t cmd;

    while (1) {
        if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY)) {
            /* Bật buzzer */
            gpio_set_level(BUZZER_GPIO, 1);
            ESP_LOGI(TAG, "Buzzer ON");

            /* Giữ buzzer trong khoảng thời gian duration_ms */
            vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));

            /* Tắt buzzer */
            gpio_set_level(BUZZER_GPIO, 0);
            ESP_LOGI(TAG, "Buzzer OFF");
        }
    }
}

/**
 * @brief Khởi tạo buzzer
 *
 * Thiết lập chân GPIO kết nối buzzer, tạo queue nhận lệnh,
 * và tạo task xử lý bật/tắt buzzer.
 *
 * @return
 *      - ESP_OK khi khởi tạo thành công
 *      - ESP_FAIL khi tạo queue không thành công
 */
esp_err_t buzzer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    /* Cấu hình GPIO */
    gpio_config(&io_conf);

    /* Đảm bảo buzzer tắt ban đầu */
    gpio_set_level(BUZZER_GPIO, 0);

    /* Tạo queue cho lệnh buzzer, kích thước queue 5 lệnh */
    buzzer_queue = xQueueCreate(5, sizeof(buzzer_cmd_t));
    if (buzzer_queue == NULL) {
        ESP_LOGE(TAG, "Không tạo được queue cho buzzer");
        return ESP_FAIL;
    }

    /* Tạo task xử lý buzzer trên core 1 */
    xTaskCreatePinnedToCore(buzzer_task, "buzzer_task", 2048, NULL, 10, NULL, 1);

    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_GPIO);
    return ESP_OK;
}

/**
 * @brief Gửi lệnh bật buzzer trong khoảng thời gian xác định
 *
 * Lệnh sẽ được gửi vào queue để task buzzer_task xử lý.
 * Nếu queue đầy, lệnh sẽ bị bỏ và trả về lỗi.
 *
 * @param duration_ms Thời gian bật buzzer, tính bằng milliseconds
 *
 * @return
 *      - ESP_OK nếu gửi lệnh thành công
 *      - ESP_FAIL nếu queue đầy, không gửi được lệnh
 */
esp_err_t buzzer_beep(int duration_ms)
{
    buzzer_cmd_t cmd = {
        .duration_ms = duration_ms
    };

    if (xQueueSend(buzzer_queue, &cmd, 0) != pdPASS) {
        ESP_LOGW(TAG, "Queue buzzer đầy, bỏ lệnh beep");
        return ESP_FAIL;
    }

    return ESP_OK;
}
