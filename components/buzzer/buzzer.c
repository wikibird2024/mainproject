/**
 * @file buzzer.c
 * @brief Driver điều khiển buzzer GPIO cho ESP32 (Active Buzzer)
 * @author Your Name/Team Name
 * @date 31/03/2025
 * @version 2.1
 * @copyright Copyright (c) 2025 Your Company. All rights reserved.
 * @note Chỉ hỗ trợ buzzer chủ động. Đối với buzzer bị động, sử dụng driver PWM riêng.
 */

/* ---------- Includes ---------- */
#include "buzzer.h"
#include "comm.h"
#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* ---------- Macro Definitions ---------- */
#define BUZZER_GPIO          GPIO_NUM_25   /**< GPIO kết nối buzzer */
#define BUZZER_QUEUE_LENGTH  5             /**< Số lệnh tối đa trong queue */

/* ---------- Static Variables ---------- */
static QueueHandle_t buzzer_queue = NULL;  /**< Queue điều khiển buzzer */

/* ---------- Struct Definitions ---------- */
typedef struct {
    int duration_ms;  /**< Thời gian bật (ms). Âm = bật vô hạn, 0 = tắt ngay */
} buzzer_cmd_t;

/* ---------- Private Functions ---------- */
static void buzzer_task(void *arg) {
    buzzer_cmd_t cmd;
    while (1) {
        if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY)) {
            if (cmd.duration_ms == 0) {
                comm_gpio_led_set(0);  // Tắt ngay
            } else {
                comm_gpio_led_set(1);  // Bật buzzer
                if (cmd.duration_ms > 0) {
                    vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));
                    comm_gpio_led_set(0);  // Tắt sau duration_ms
                }
            }
        }
    }
}

/* ---------- Public Functions ---------- */
esp_err_t buzzer_init(void) {
    // Khởi tạo GPIO qua comm.h
    esp_err_t ret = comm_gpio_init(BUZZER_GPIO, -1);
    if (ret != ESP_OK) {
        ERROR("BUZZER", "Failed to initialize GPIO %d", BUZZER_GPIO);
        return ret;
    }
    comm_gpio_led_set(0);  // Tắt ban đầu

    // Tạo queue
    buzzer_queue = xQueueCreate(BUZZER_QUEUE_LENGTH, sizeof(buzzer_cmd_t));
    if (buzzer_queue == NULL) {
        ERROR("BUZZER", "Failed to create queue");
        return ESP_FAIL;
    }

    // Tạo task
    BaseType_t task_ret = xTaskCreatePinnedToCore(
        buzzer_task, "buzzer_task", 1024, NULL, 5, NULL, 1
    );
    if (task_ret != pdPASS) {
        ERROR("BUZZER", "Failed to create buzzer task");
        vQueueDelete(buzzer_queue);
        buzzer_queue = NULL;
        return ESP_FAIL;
    }

    INFO("BUZZER", "Initialized (GPIO %d)", BUZZER_GPIO);
    return ESP_OK;
}

esp_err_t buzzer_beep(int duration_ms) {
    if (buzzer_queue == NULL) {
        ERROR("BUZZER", "Queue not initialized");
        return ESP_FAIL;
    }

    buzzer_cmd_t cmd = { .duration_ms = duration_ms };
    BaseType_t ret = xQueueSend(buzzer_queue, &cmd, 0);
    
    if (ret != pdPASS) {
        WARN("BUZZER", "Queue full, overwriting...");
        xQueueOverwrite(buzzer_queue, &cmd);
    }

    return ESP_OK;
}

esp_err_t buzzer_stop(void) {
    esp_err_t ret = buzzer_beep(0);
    if (ret == ESP_OK) {
        INFO("BUZZER", "Buzzer stopped");
    }
    return ret;
}