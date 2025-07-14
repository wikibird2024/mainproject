/**
 * @file comm.h
 * @brief Interface for hardware communication: UART, I2C, PWM, GPIO.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes for communication operations.
 */
typedef enum {
    COMM_SUCCESS = 0,
    COMM_TIMEOUT = 1,
    COMM_INVALID_PARAM = 2
} comm_result_t;

/** UART Functions */
void comm_uart_init(void);
comm_result_t comm_uart_send_command(const char *command, char *response_buf, size_t buf_size);

/** I2C Functions */
esp_err_t comm_i2c_init(void);
esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data);

/** PWM Functions */
esp_err_t comm_pwm_init(int pin, int frequency);
esp_err_t comm_pwm_set_duty_cycle(int duty_cycle);
esp_err_t comm_pwm_stop(void);

/** GPIO Functions */
esp_err_t comm_gpio_init(int led_pin, int button_pin);
esp_err_t comm_gpio_led_set(int state);
esp_err_t comm_gpio_button_read(bool *pressed);

#ifdef __cplusplus
}
#endif
