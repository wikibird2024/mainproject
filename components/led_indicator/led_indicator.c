
/**
 * @file led_indicator.c
 * @brief Triển khai điều khiển LED đa luồng an toàn, hỗ trợ nhấp nháy với các chế độ khác nhau
 */

#include "led_indicator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include <stdlib.h>

/**
 * @brief Tham số cho nhiệm vụ nhấp nháy LED
 */
typedef struct {
    uint32_t delay_ms;       ///< Tổng thời gian nhấp nháy (ms)
    led_blink_mode_t mode;   ///< Chế độ nhấp nháy (1, 2 hoặc 3 lần)
} led_blink_param_t;

/**
 * @brief Ngữ cảnh điều khiển LED, lưu trạng thái và tài nguyên
 */
static struct {
    int gpio_num;                    ///< Số GPIO điều khiển LED
    bool active_high;                ///< LED bật ở mức cao hay thấp
    TaskHandle_t blink_task;         ///< Handle task nhấp nháy LED
    SemaphoreHandle_t mutex;         ///< Mutex bảo vệ truy cập song song
} led_ctx = {
    .gpio_num = -1,
    .blink_task = NULL,
    .mutex = NULL
};

/**
 * @brief Đặt trạng thái LED theo logic active_high
 * @param state true: bật LED, false: tắt LED
 */
static void set_led_state(bool state) {
    gpio_set_level(led_ctx.gpio_num, led_ctx.active_high ? state : !state);
}

/**
 * @brief Các hàm tiện ích để gọi với with_mutex
 */
static void set_led_off(void) {
    set_led_state(false);
}

static void set_led_on(void) {
    set_led_state(true);
}

/**
 * @brief Nhấp nháy LED theo mẫu, số lần xác định
 * @param delay_ms Tổng thời gian nhấp nháy (ms)
 * @param count Số lần nhấp nháy trong mẫu
 */
static void blink_pattern(uint32_t delay_ms, int count) {
    for (int i = 0; i < count; ++i) {
        set_led_state(true);
        vTaskDelay(pdMS_TO_TICKS(delay_ms / (2 * count)));
        set_led_state(false);
        vTaskDelay(pdMS_TO_TICKS(delay_ms / (2 * count)));
    }
}

/**
 * @brief Task thực thi việc nhấp nháy LED liên tục theo tham số truyền vào
 * @param arg Con trỏ đến led_blink_param_t
 */
static void blink_task(void *arg) {
    led_blink_param_t *params = (led_blink_param_t *)arg;
    uint32_t delay_ms = params->delay_ms;
    led_blink_mode_t mode = params->mode;
    free(params);

    while (true) {
        switch (mode) {
            case LED_BLINK_SINGLE:
                blink_pattern(delay_ms, 1);
                break;
            case LED_BLINK_DOUBLE:
                blink_pattern(delay_ms, 2);
                break;
            case LED_BLINK_TRIPLE:
                blink_pattern(delay_ms, 3);
                break;
            default:
                // Nếu mode không hợp lệ, giữ trạng thái LED tắt
                set_led_state(false);
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
                break;
        }
    }
}

/**
 * @brief Khởi tạo LED indicator với cấu hình GPIO và logic active high/low
 * @param config Con trỏ tới cấu hình bao gồm gpio_num và active_high
 * @return ESP_OK nếu thành công, lỗi khác nếu sai tham số hoặc không cấp phát được mutex
 */
esp_err_t led_indicator_init(const led_indicator_config_t *config) {
    if (!config || config->gpio_num < 0) return ESP_ERR_INVALID_ARG;

    if (!led_ctx.mutex) {
        led_ctx.mutex = xSemaphoreCreateMutex();
        if (!led_ctx.mutex) return ESP_ERR_NO_MEM;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << config->gpio_num,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) return ret;

    led_ctx.gpio_num = config->gpio_num;
    led_ctx.active_high = config->active_high;
    set_led_state(false);

    return ESP_OK;
}

/**
 * @brief Bắt đầu nhấp nháy LED với thời gian trễ và chế độ xác định
 * @param delay_ms Thời gian nhấp nháy tổng (ms), trong khoảng 10 đến 10000
 * @param mode Chế độ nhấp nháy (đơn, đôi, ba lần)
 * @return ESP_OK nếu thành công, lỗi khác nếu tham số không hợp lệ hoặc không tạo được task
 */
esp_err_t led_indicator_start_blink(uint32_t delay_ms, led_blink_mode_t mode) {
    if (delay_ms < 10 || delay_ms > 10000 || led_ctx.gpio_num < 0)
        return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(led_ctx.mutex, portMAX_DELAY);

    if (led_ctx.blink_task) {
        vTaskDelete(led_ctx.blink_task);
        led_ctx.blink_task = NULL;
    }

    led_blink_param_t *params = malloc(sizeof(led_blink_param_t));
    if (!params) {
        xSemaphoreGive(led_ctx.mutex);
        return ESP_ERR_NO_MEM;
    }

    params->delay_ms = delay_ms;
    params->mode = mode;

    BaseType_t res = xTaskCreate(
        blink_task,
        "led_blink",
        2048,
        params,
        5,
        &led_ctx.blink_task
    );

    xSemaphoreGive(led_ctx.mutex);
    return (res == pdPASS) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief Dừng task nhấp nháy LED nếu đang chạy
 */
static void stop_blink_task(void) {
    if (led_ctx.blink_task) {
        vTaskDelete(led_ctx.blink_task);
        led_ctx.blink_task = NULL;
    }
}

/**
 * @brief Thực thi một hàm trong vùng bảo vệ mutex, đảm bảo truy cập đồng bộ
 * @param func Con trỏ hàm không tham số và không trả về
 */
static void with_mutex(void (*func)(void)) {
    xSemaphoreTake(led_ctx.mutex, portMAX_DELAY);
    func();
    xSemaphoreGive(led_ctx.mutex);
}

/**
 * @brief Dừng nhấp nháy LED và tắt LED
 */
void led_indicator_stop_blink(void) {
    with_mutex(stop_blink_task);
    with_mutex(set_led_off);
}

/**
 * @brief Bật LED và dừng nhấp nháy nếu có
 */
void led_indicator_on(void) {
    with_mutex(stop_blink_task);
    with_mutex(set_led_on);
}

/**
 * @brief Tắt LED và dừng nhấp nháy nếu có
 */
void led_indicator_off(void) {
    with_mutex(stop_blink_task);
    with_mutex(set_led_off);
}
