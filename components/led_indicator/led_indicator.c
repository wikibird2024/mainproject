
#include "led_indicator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static int led_gpio = -1;
static TaskHandle_t blink_task_handle = NULL;

static void led_blink_task(void *param) {
    uint32_t delay_ms = (uint32_t)param;
    while (1) {
        gpio_set_level(led_gpio, 1);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        gpio_set_level(led_gpio, 0);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t led_indicator_init(int gpio_num) {
    led_gpio = gpio_num;
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << led_gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret == ESP_OK) {
        gpio_set_level(led_gpio, 0); // tắt LED lúc khởi tạo
    }
    return ret;
}

esp_err_t led_indicator_start_blink(uint32_t delay_ms) {
    if (blink_task_handle != NULL) {
        // Task đang chạy rồi, không tạo thêm
        return ESP_ERR_INVALID_STATE;
    }

    BaseType_t ret = xTaskCreate(
        led_blink_task,
        "led_blink_task",
        1024,
        (void *)delay_ms,
        5,
        &blink_task_handle
    );

    return (ret == pdPASS) ? ESP_OK : ESP_FAIL;
}

void led_indicator_stop_blink(void) {
    if (blink_task_handle != NULL) {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
        gpio_set_level(led_gpio, 0); // tắt led khi dừng
    }
}

void led_indicator_on(void) {
    if (led_gpio >= 0) {
        gpio_set_level(led_gpio, 1);
    }
}

void led_indicator_off(void) {
    if (led_gpio >= 0) {
        gpio_set_level(led_gpio, 0);
    }
}
