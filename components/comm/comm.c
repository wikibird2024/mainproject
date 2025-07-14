/**
 * @file comm.c
 * @brief Hardware communication driver for modules: UART (SIM 4G), I2C (MPU6050), PWM (Buzzer), GPIO (LED & Button).
 */

#include "comm.h"
#include "comm_log.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

#define UART_NUM        UART_NUM_1
#define UART_TX_PIN     17
#define UART_RX_PIN     16
#define I2C_MASTER_PORT 0
#define I2C_MASTER_SCL  22
#define I2C_MASTER_SDA  21

static int pwm_pin = -1;
static int led_gpio = -1;
static int button_gpio = -1;

static bool uart_initialized = false;
static bool i2c_initialized = false;
static bool pwm_initialized = false;
static bool gpio_initialized = false;

/* ================= UART ================= */

void comm_uart_init(void) {
    if (uart_initialized) return;
    uart_initialized = true;

    COMM_LOGI("Initializing UART...");

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM, 2048, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    COMM_LOGI("UART initialized");
}

comm_result_t comm_uart_send_command(const char *command, char *response_buf, size_t buf_size) {
    if (!command) return COMM_INVALID_PARAM;

    uart_flush_input(UART_NUM);
    uart_write_bytes(UART_NUM, command, strlen(command));
    uart_write_bytes(UART_NUM, "\r\n", 2);

    if (response_buf && buf_size > 0) {
        int read_bytes = uart_read_bytes(UART_NUM, (uint8_t *)response_buf, buf_size - 1, pdMS_TO_TICKS(200));
        if (read_bytes > 0) {
            response_buf[read_bytes] = '\0';
            COMM_LOGI("UART response: %s", response_buf);
            return COMM_SUCCESS;
        } else {
            response_buf[0] = '\0';
            return COMM_TIMEOUT;
        }
    }

    return COMM_SUCCESS;
}

/* ================= I2C ================= */

esp_err_t comm_i2c_init(void) {
    if (i2c_initialized) return ESP_OK;
    i2c_initialized = true;

    COMM_LOGI("Initializing I2C");

    const i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    return i2c_master_bus_init(&bus_config);
}

esp_err_t comm_i2c_write(uint8_t addr, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(I2C_MASTER_PORT, addr, buf, 2, pdMS_TO_TICKS(100));
}

esp_err_t comm_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;
    esp_err_t err = i2c_master_transmit(I2C_MASTER_PORT, addr, &reg, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return err;
    return i2c_master_receive(I2C_MASTER_PORT, addr, data, 1, pdMS_TO_TICKS(100));
}

/* ================= PWM ================= */

esp_err_t comm_pwm_init(int pin, int freq) {
    if (pin < 0) return ESP_ERR_INVALID_ARG;
    if (pwm_initialized) return ESP_OK;
    pwm_initialized = true;

    pwm_pin = pin;
    COMM_LOGI("Initializing PWM on GPIO %d @ %dHz", pin, freq);

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = freq,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num = pin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));

    return ESP_OK;
}

esp_err_t comm_pwm_set_duty_cycle(int duty) {
    if (duty < 0 || duty > 1023) return ESP_ERR_INVALID_ARG;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty));
    return ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

esp_err_t comm_pwm_stop(void) {
    return comm_pwm_set_duty_cycle(0);
}

/* ================= GPIO ================= */

esp_err_t comm_gpio_init(int led, int button) {
    if (led < 0 || button < 0) return ESP_ERR_INVALID_ARG;
    if (gpio_initialized) return ESP_OK;
    gpio_initialized = true;

    led_gpio = led;
    button_gpio = button;

    gpio_config_t led_conf = {
        .pin_bit_mask = 1ULL << led,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);

    gpio_config_t button_conf = {
        .pin_bit_mask = 1ULL << button,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_conf);

    COMM_LOGI("GPIO initialized: LED=%d, BTN=%d", led, button);
    return ESP_OK;
}

esp_err_t comm_gpio_led_set(int state) {
    if (led_gpio < 0) return ESP_FAIL;
    return gpio_set_level(led_gpio, state);
}

esp_err_t comm_gpio_button_read(bool *pressed) {
    if (!pressed || button_gpio < 0) return ESP_ERR_INVALID_ARG;
    int level = gpio_get_level(button_gpio);
    *pressed = (level == 0);
    return ESP_OK;
}