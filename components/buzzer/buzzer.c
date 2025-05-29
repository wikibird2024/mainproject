#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define BUZZER_GPIO GPIO_NUM_25   // Chan GPIO  ket noi buzzer

static const char *TAG = "buzzer";
static QueueHandle_t buzzer_queue = NULL;

// Cau truc lenh bat buzzer
typedef struct {
    int duration_ms;
} buzzer_cmd_t;

// Task xu ly bat/tat buzzer theo lenh tu queue
static void buzzer_task(void *arg) {
    buzzer_cmd_t cmd;
    while (1) {
        if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY)) {
            gpio_set_level(BUZZER_GPIO, 1);
            ESP_LOGI(TAG, "Buzzer ON");
            vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));
            gpio_set_level(BUZZER_GPIO, 0);
            ESP_LOGI(TAG, "Buzzer OFF");
        }
    }
}

// Ham khoi tạo buzzer: config GPIO, tạo queue, tao task
esp_err_t buzzer_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(BUZZER_GPIO, 0); // Tat buzzer ban dau

    buzzer_queue = xQueueCreate(5, sizeof(buzzer_cmd_t));
    if (buzzer_queue == NULL) {
        ESP_LOGE(TAG, "Không tạo được queue cho buzzer");
        return ESP_FAIL;
    }

    xTaskCreatePinnedToCore(buzzer_task, "buzzer_task", 2048, NULL, 10, NULL, 1);
    ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_GPIO);
    return ESP_OK;
}

// Gui lenh bat buzzer (non-blocking)
esp_err_t buzzer_beep(int duration_ms) {
    buzzer_cmd_t cmd = {
        .duration_ms = duration_ms
    };

    if (xQueueSend(buzzer_queue, &cmd, 0) != pdPASS){
        ESP_LOGW(TAG, "Queue buzzer đầy, bỏ lệnh beep");
        return ESP_FAIL;
    }

    return ESP_OK;
}
