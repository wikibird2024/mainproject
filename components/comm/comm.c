#include "comm.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

#define TAG "COMM"

// UART Cau hinh
#define UART_NUM UART_NUM_1
#define UART_TX_PIN 17
#define UART_RX_PIN 16

// I2C Cau hinh
#define I2C_MASTER_SCL 22
#define I2C_MASTER_SDA 21

// PWM Cau hinh
static int pwm_pin = -1;

// GPIO LED & BUTTON
static int led_gpio = -1;
static int button_gpio = -1;

// ======== UART (SIM 4G GPS) ========
void comm_uart_init() {
    ESP_LOGI(TAG, "Initializing UART...");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(TAG, "UART initialized.");
}

bool comm_uart_send_command(const char *command, char *response_buf, size_t buf_size) {
    if (!command) return false;

    ESP_LOGI(TAG, "Sending command: %s", command);
    uart_write_bytes(UART_NUM, command, strlen(command));
    uart_write_bytes(UART_NUM, "\r\n", 2);

    if (response_buf && buf_size > 0) {
        int read_bytes = uart_read_bytes(UART_NUM, (uint8_t *)response_buf, buf_size - 1, 100 / portTICK_PERIOD_MS);
        if (read_bytes > 0) {
            response_buf[read_bytes] = '\0';
            ESP_LOGI(TAG, "Received response: %s", response_buf);
            return true;
        } else {
            ESP_LOGW(TAG, "No response received");
            response_buf[0] = '\0';
            return false;
        }
    }

    return true;
}

// ======== I2C (MPU6050) ========
void comm_i2c_init() {
    ESP_LOGI(TAG, "Initializing I2C...");
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    ESP_LOGI(TAG, "I2C initialized.");
}

esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
    uint8_t buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_NUM_0, device_addr, buf, sizeof(buf), 100 / portTICK_PERIOD_MS);
}

esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;
    return i2c_master_write_read_device(I2C_NUM_0, device_addr, &reg_addr, 1, data, 1, 100 / portTICK_PERIOD_MS);
}

// ======== PWM (Buzzer or Led ) ========
esp_err_t comm_pwm_init(int pin, int frequency) {
    if (pin < 0) return ESP_ERR_INVALID_ARG;
    pwm_pin = pin;

    ESP_LOGI(TAG, "Initializing PWM on GPIO %d with %d Hz", pwm_pin, frequency);

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = pwm_pin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    return ESP_OK;
}

esp_err_t comm_pwm_set_duty_cycle(int duty_cycle) {
    if (duty_cycle < 0 || duty_cycle > 1023) return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "Set PWM duty: %d", duty_cycle);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
    return ESP_OK;
}

esp_err_t comm_pwm_stop(void) {
    ESP_LOGI(TAG, "Stopping PWM");
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
    return ESP_OK;
}

// ======== GPIO (LED & Button) ========
esp_err_t comm_gpio_init(int led_pin, int button_pin) {
    if (led_pin < 0 || button_pin < 0) return ESP_ERR_INVALID_ARG;

    led_gpio = led_pin;
    button_gpio = button_pin;

    gpio_reset_pin(led_gpio);
    gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);

    gpio_reset_pin(button_gpio);
    gpio_set_direction(button_gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(button_gpio, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "LED GPIO: %d, Button GPIO: %d initialized", led_gpio, button_gpio);
    return ESP_OK;
}

esp_err_t comm_gpio_led_set(int state) {
    if (led_gpio < 0) return ESP_FAIL;
    gpio_set_level(led_gpio, state);
    return ESP_OK;
}

esp_err_t comm_gpio_button_read(bool *pressed) {
    if (!pressed || button_gpio < 0) return ESP_ERR_INVALID_ARG;
    int level = gpio_get_level(button_gpio);
    *pressed = (level == 0); // Active low
    return ESP_OK;
}
