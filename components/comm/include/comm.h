#pragma once
#include "esp_err.h"
#include "sdkconfig.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// UART Configuration
#define UART_NUM CONFIG_UART_PORT_NUM
#define UART_TX_PIN CONFIG_UART_TX_PIN
#define UART_RX_PIN CONFIG_UART_RX_PIN

// I2C Configuration
#define I2C_MASTER_PORT CONFIG_I2C_PORT_NUM
#define I2C_MASTER_SDA CONFIG_I2C_SDA_PIN
#define I2C_MASTER_SCL CONFIG_I2C_SCL_PIN
#define I2C_DEVICE_ADDR_DEFAULT CONFIG_I2C_DEVICE_ADDR

// GPIO Configuration
#define DEFAULT_LED_GPIO CONFIG_DEFAULT_LED_GPIO
#define DEFAULT_BUTTON_GPIO CONFIG_DEFAULT_BUTTON_GPIO

typedef enum {
  COMM_SUCCESS = 0,
  COMM_TIMEOUT = 1,
  COMM_INVALID_PARAM = 2
} comm_result_t;

/* Function declarations remain unchanged from your original file */
esp_err_t comm_init_all(void);

esp_err_t comm_uart_init(void);

comm_result_t comm_uart_send_command(const char *command, char *response_buf,
                                     size_t buf_size);
esp_err_t comm_i2c_init(void);
esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data);
esp_err_t comm_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data);
esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *data,
                        size_t size);
esp_err_t comm_i2c_read_byte(uint8_t addr, uint8_t reg, uint8_t *data);

esp_err_t comm_pwm_init(int pin, int freq);
esp_err_t comm_pwm_set_duty_cycle(int duty);
esp_err_t comm_pwm_stop(void);

esp_err_t comm_gpio_init(int led_pin, int button_pin);
esp_err_t comm_gpio_led_set(int state);
esp_err_t comm_gpio_button_read(bool *pressed);

#ifdef __cplusplus
}
#endif
