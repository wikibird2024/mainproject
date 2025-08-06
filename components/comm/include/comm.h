/**
 * @file comm.h
 * @brief Public interface for the low-level communication driver.
 */

#pragma once

#include "esp_err.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// A small constant for buffer size
#define UART_BUFFER_SIZE 1024

// Result enum for communication functions
typedef enum {
    COMM_SUCCESS = 0,
    COMM_INVALID_PARAM,
    COMM_TIMEOUT,
    COMM_ERROR
} comm_result_t;

// UART configuration from Kconfig
#define UART_NUM CONFIG_COMM_UART_PORT_NUM
#define UART_TX_PIN CONFIG_COMM_UART_TX_PIN
#define UART_RX_PIN CONFIG_COMM_UART_RX_PIN
#define UART_BAUD_RATE CONFIG_COMM_UART_BAUD_RATE

// I2C configuration from Kconfig
#define I2C_MASTER_PORT CONFIG_COMM_I2C_PORT_NUM
#define I2C_MASTER_SDA CONFIG_COMM_I2C_SDA_PIN
#define I2C_MASTER_SCL CONFIG_COMM_I2C_SCL_PIN
#define I2C_CLOCK_SPEED CONFIG_COMM_I2C_CLOCK_SPEED
#define I2C_DEVICE_ADDR CONFIG_COMM_I2C_DEVICE_ADDR

// GPIO configuration from Kconfig
#define DEFAULT_LED_GPIO CONFIG_COMM_DEFAULT_LED_GPIO
#define DEFAULT_BUTTON_GPIO CONFIG_COMM_DEFAULT_BUTTON_GPIO

// --- Public function declarations ---

// Initializes all communication interfaces (UART, I2C, GPIO)
esp_err_t comm_init_all(void);

// UART Functions
esp_err_t comm_uart_init(int uart_num, int tx_pin, int rx_pin);
comm_result_t comm_uart_send_command(const char *command, char *response_buf,
                                     size_t buf_size, int timeout_ms);

// I2C Functions
esp_err_t comm_i2c_init(void);
esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
esp_err_t comm_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data);
esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *data,
                        size_t size);
esp_err_t comm_i2c_read_byte(uint8_t addr, uint8_t reg, uint8_t *data);

// PWM Functions
esp_err_t comm_pwm_init(int pin, int freq);
esp_err_t comm_pwm_set_duty_cycle(int duty);
esp_err_t comm_pwm_stop(void);

// GPIO Functions
esp_err_t comm_gpio_init(int led_pin, int button_pin);
esp_err_t comm_gpio_led_set(int state);
esp_err_t comm_gpio_button_read(bool *pressed);


#ifdef __cplusplus
}
#endif
