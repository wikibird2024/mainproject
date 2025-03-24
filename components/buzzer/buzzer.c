#include "buzzer.h"
#include "esp_log.h"

static const char *TAG = "buzzer";

/**
 * @brief Khởi tạo buzzer (Cấu hình GPIO output)
 */
void buzzer_init(void) {
    gpio_pad_select_gpio(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 0); // Tắt còi ban đầu
    ESP_LOGI(TAG, "Buzzer initialized at GPIO %d", BUZZER_GPIO);
}

/**
 * @brief Bật còi
 */
void buzzer_on(void) {
    gpio_set_level(BUZZER_GPIO, 1);
    ESP_LOGI(TAG, "Buzzer ON");
}

/**
 * @brief Tắt còi
 */
void buzzer_off(void) {
    gpio_set_level(BUZZER_GPIO, 0);
    ESP_LOGI(TAG, "Buzzer OFF");
}

