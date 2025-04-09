#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/// ======== UART (SIM 4G GPS) ========

void comm_uart_init(void);                                 // Khởi tạo UART
bool comm_uart_send_command(const char *command, char *response_buf, size_t buf_size); // Gửi lệnh AT và nhận phản hồi

/// ======== I2C (MPU6050) ========

void comm_i2c_init(void);                                  // Khởi tạo I2C
esp_err_t comm_i2c_write(uint8_t dev, uint8_t reg, uint8_t data); // Ghi 1 byte
esp_err_t comm_i2c_read(uint8_t dev, uint8_t reg, uint8_t *data); // Đọc 1 byte

/// ======== PWM (Buzzer) ========

esp_err_t comm_pwm_init(int pin, int freq);                // Khởi tạo PWM
esp_err_t comm_pwm_set_duty_cycle(int duty);               // Thiết lập duty
esp_err_t comm_pwm_stop(void);                             // Dừng PWM

/// ======== GPIO (LED & Button) ========

esp_err_t comm_gpio_init(int led_pin, int button_pin);     // Khởi tạo GPIO
esp_err_t comm_gpio_led_set(int state);                    // Bật/tắt LED
esp_err_t comm_gpio_button_read(bool *pressed);            // Đọc nút nhấn
