
#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// ======== UART (SIM 4G GPS) ========
void comm_uart_init(void);
esp_err_t comm_uart_send_command(const char *command);
esp_err_t comm_uart_read_response(char *response, int len);

// ======== I2C (MPU6050) ========
void comm_i2c_init(void);
esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data);

// ======== PWM (Buzzer) ========
void comm_pwm_init(int pin, int frequency);
esp_err_t comm_pwm_set_duty_cycle(int duty_cycle);

// ======== GPIO (LED & Button) ========
void comm_gpio_init(int led_pin, int button_pin);
void comm_gpio_led_set(int state);
bool comm_gpio_button_read(void);
