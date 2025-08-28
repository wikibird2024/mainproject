/**
 * @file comm.h
 * @brief Public API for the low-level communication drivers.
 *
 * This module provides a Hardware Abstraction Layer (HAL) to simplify
 * interactions with various communication interfaces such as UART, I2C, and
 * GPIO. This design allows higher-level modules to communicate with hardware
 * without needing to know the low-level implementation details.
 *
 * @author Hao Tran
 * @date 2025
 */

#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

// A small constant for buffer size
#define UART_BUFFER_SIZE 1024

/**
 * @brief Defines the result codes for communication functions.
 */
typedef enum {
  COMM_SUCCESS = 0,   /**< Operation was successful */
  COMM_INVALID_PARAM, /**< An invalid parameter was provided */
  COMM_TIMEOUT,       /**< The operation timed out */
  COMM_ERROR          /**< An unspecified error occurred */
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

/**
 * @brief Initializes all communication interfaces.
 *
 * This function sets up all necessary drivers for UART, I2C, and GPIO.
 *
 * @return
 * - ESP_OK: All interfaces were initialized successfully.
 * - An ESP-IDF-specific error code: If an initialization failure occurred.
 */
esp_err_t comm_init_all(void);

// UART Functions
/**
 * @brief Initializes the UART driver.
 *
 * @param uart_num The UART port number to initialize.
 * @param tx_pin The GPIO pin for the TX line.
 * @param rx_pin The GPIO pin for the RX line.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_uart_init(int uart_num, int tx_pin, int rx_pin);

/**
 * @brief Sends a command via UART and waits for a response.
 *
 * Sends a null-terminated string and waits for a response within a timeout
 * period.
 *
 * @param command A pointer to the command string to send.
 * @param response_buf A pointer to the buffer to store the response.
 * @param buf_size The size of the response buffer.
 * @param timeout_ms The maximum wait time in milliseconds.
 * @return comm_result_t The result of the operation (success, timeout, or
 * error).
 */
comm_result_t comm_uart_send_command(const char *command, char *response_buf,
                                     size_t buf_size, int timeout_ms);

// I2C Functions
/**
 * @brief Initializes the I2C master driver.
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_i2c_init(void);

/**
 * @brief Writes a single byte of data to a device's register via I2C.
 *
 * @param device_addr The 7-bit address of the I2C device.
 * @param reg_addr The address of the register to write to.
 * @param data The byte of data to write.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data);

/**
 * @brief Writes a single byte of data to a specific register on an I2C device.
 * @param addr The device address.
 * @param reg The register address.
 * @param data The data to write.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data);

/**
 * @brief Reads a block of data from an I2C device.
 *
 * @param device_addr The 7-bit address of the device.
 * @param reg The starting register address to read from.
 * @param data A pointer to the buffer where the read data will be stored.
 * @param size The number of bytes to read.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *data,
                        size_t size);

/**
 * @brief Reads a single byte of data from a specific I2C register.
 * @param addr The device address.
 * @param reg The register address.
 * @param data A pointer to where the read data will be stored.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_i2c_read_byte(uint8_t addr, uint8_t reg, uint8_t *data);

// PWM Functions
/**
 * @brief Initializes PWM on a specific GPIO pin.
 *
 * @param pin The GPIO pin to use.
 * @param freq The frequency of the PWM signal in Hz.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_pwm_init(int pin, int freq);

/**
 * @brief Sets the duty cycle for the PWM signal.
 *
 * @param duty The duty cycle value (0 to 100).
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_pwm_set_duty_cycle(int duty);

/**
 * @brief Stops the PWM signal.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_pwm_stop(void);

// GPIO Functions
/**
 * @brief Initializes the GPIO pins for the LED and button.
 *
 * @param led_pin The GPIO pin for the LED.
 * @param button_pin The GPIO pin for the button.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_gpio_init(int led_pin, int button_pin);

/**
 * @brief Sets the state of the LED.
 *
 * @param state The new state of the LED (0 for OFF, 1 for ON).
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_gpio_led_set(int state);

/**
 * @brief Reads the state of the button.
 *
 * @param pressed A pointer to a boolean to store the button's state.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t comm_gpio_button_read(bool *pressed);

#ifdef __cplusplus
}
#endif
