/**
 * @file led_indicator.c
 * @brief Implementation of single LED indicator with multiple blink modes.
 */

#include "led_indicator.h"
#include "comm.h"
#include "debugs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_INDICATOR_GPIO       2      // GPIO connected to LED
#define LED_TASK_STACK_SIZE      2048
#define LED_TASK_PRIORITY        4

#define FAST_BLINK_PERIOD_MS     200
#define SLOW_BLINK_PERIOD_MS     1000
#define ERROR_BLINK_ON_MS        150
#define ERROR_BLINK_OFF_MS       150
#define ERROR_BLINK_PAUSE_MS     700

static TaskHandle_t led_task_handle = NULL;
static led_mode_t current_mode = LED_MODE_OFF;
static bool led_initialized = false;

static void led_set(bool on)
{
    comm_gpio_led_set(on ? 1 : 0);
}

static void led_task(void *arg)
{
    while (1) {
        switch (current_mode) {
            case LED_MODE_OFF:
                led_set(false);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_MODE_ON:
                led_set(true);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_MODE_BLINK_FAST:
                led_set(true);
                vTaskDelay(pdMS_TO_TICKS(FAST_BLINK_PERIOD_MS / 2));
                led_set(false);
                vTaskDelay(pdMS_TO_TICKS(FAST_BLINK_PERIOD_MS / 2));
                break;

            case LED_MODE_BLINK_SLOW:
                led_set(true);
                vTaskDelay(pdMS_TO_TICKS(SLOW_BLINK_PERIOD_MS / 2));
                led_set(false);
                vTaskDelay(pdMS_TO_TICKS(SLOW_BLINK_PERIOD_MS / 2));
                break;

            case LED_MODE_BLINK_ERROR:
                for (int i = 0; i < 2; ++i) {
                    led_set(true);
                    vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_ON_MS));
                    led_set(false);
                    vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_OFF_MS));
                }
                vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_PAUSE_MS));
                break;

            default:
                led_set(false);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
    }
}

esp_err_t led_indicator_init(void)
{
    if (led_initialized) {
        DEBUGS_LOGI("LED already initialized.");
        return ESP_OK;
    }

    esp_err_t err = comm_gpio_init(LED_INDICATOR_GPIO, -1);  // No button
    if (err != ESP_OK) {
        DEBUGS_LOGE("LED GPIO init failed: %s", esp_err_to_name(err));
        return err;
    }

    if (xTaskCreate(led_task, "led_indicator_task", LED_TASK_STACK_SIZE, NULL,
                    LED_TASK_PRIORITY, &led_task_handle) != pdPASS) {
        DEBUGS_LOGE("Failed to create LED indicator task");
        return ESP_FAIL;
    }

    led_initialized = true;
    DEBUGS_LOGI("LED indicator initialized on GPIO %d", LED_INDICATOR_GPIO);
    return ESP_OK;
}

esp_err_t led_indicator_set_mode(led_mode_t mode)
{
    if (!led_initialized) {
        DEBUGS_LOGW("LED indicator not initialized.");
        return ESP_ERR_INVALID_STATE;
    }

    if (mode < LED_MODE_OFF || mode > LED_MODE_BLINK_ERROR) {
        DEBUGS_LOGW("Invalid LED mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }

    current_mode = mode;
    DEBUGS_LOGI("LED mode set to %d", mode);
    return ESP_OK;
}

void led_indicator_deinit(void)
{
    if (!led_initialized) return;

    vTaskDelete(led_task_handle);
    led_set(false);
    led_task_handle = NULL;
    led_initialized = false;

    DEBUGS_LOGI("LED indicator deinitialized.");
}
