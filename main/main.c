/**
 * @file main.c
 * @brief Fall detection system using ESP32, MPU6050, and EC800K 4G-GPS module.
 *
 * @details This application detects falls via MPU6050 sensor data,
 * sends GPS coordinates via SMS using a Quectel EC800K module,
 * and provides local alerts using a buzzer and LED.
 *
 * @author
 *  Embedded Systems Team - Fall Alert Project
 *
 * @date
 *  Created: 2025-06-01
 *  Updated: 2025-06-01
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

// --- Configuration constants ---
#define LED_GPIO             2                   /**< GPIO pin for LED indicator */
#define CHECK_INTERVAL_MS    1000                /**< Fall detection check interval (ms) */
#define ALERT_DURATION_MS    8000                /**< Alert duration after fall (ms) */
#define LED_BLINK_PERIOD     100                 /**< LED blink period (ms) */
#define MAX_RETRY            3                   /**< Max retry count for resource creation */
#define MUTEX_TIMEOUT_MS     1000                /**< Mutex wait timeout (ms) */

static const char *PHONE_NUMBER = "+84559865843"; /**< Preconfigured emergency phone number */

// --- System events enumeration ---
/**
 * @brief System events used in the event queue.
 */
typedef enum {
    FALL_DETECTED     /**< Event triggered when a fall is detected */
} system_event_t;

// --- Global objects ---
static QueueHandle_t xEventQueue = NULL;  /**< Queue for system events */
static SemaphoreHandle_t xMutex = NULL;   /**< Mutex for shared I2C access */

/**
 * @brief Callback function for SMS send result.
 *
 * @param success true if SMS was sent successfully; false otherwise.
 */
void sms_callback(bool success) {
    if (success) {
        INFO("Gửi SMS thành công");
    } else {
        ERROR("Gửi SMS thất bại");
    }
}

/**
 * @brief Initialize the system components and resources.
 *
 * @note This function must be called before any tasks are started.
 *       Will restart the system on failure.
 */
void system_init(void) {
    INFO("Khởi tạo hệ thống...");

    // Create mutex with retry
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

    // Create event queue
    xEventQueue = xQueueCreate(5, sizeof(system_event_t));
    if (!xEventQueue) {
        ERROR("Tạo queue thất bại");
        esp_restart();
    }

    // Initialize subsystems
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
 * @brief Task that continuously reads sensor data and detects falls.
 *
 * @param[in] param Unused (pass NULL).
 *
 * @note Uses mutex to access MPU6050 safely.
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
 * @brief Task that handles system alerts (SMS, buzzer, LED) upon event.
 *
 * @param[in] param Unused (pass NULL).
 */
void alert_task(void *param) {
    INFO("MAIN", "Bắt đầu task cảnh báo");
    system_event_t event;
    while (1) {
        if (xQueueReceive(xEventQueue, &event, portMAX_DELAY)) {
            if (event == FALL_DETECTED) {
                INFO("MAIN", "Xử lý cảnh báo té ngã...");
                sim4g_gps_data_t location = sim4g_gps_get_location();
                if (!location.valid) {
                    WARN("MAIN", "Không thể lấy vị trí GPS");
                }
                sim4g_gps_send_fall_alert_async(&location, sms_callback);
                buzzer_beep(ALERT_DURATION_MS);
                led_indicator_start_blink(LED_BLINK_PERIOD, LED_BLINK_DOUBLE);
                vTaskDelay(pdMS_TO_TICKS(ALERT_DURATION_MS));
                led_indicator_stop_blink();
                buzzer_stop();
            }
        }
    }
}
/**
 * @brief Entry point for application. Initializes system and launches tasks.
 *
 * @note Will restart the system if task creation fails.
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