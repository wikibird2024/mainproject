
#include "comm.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TAG "COMM"

// ======== GPIO & PIN CONFIGURATION ========
#define UART_NUM UART_NUM_1
#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define I2C_MASTER_SCL 22
#define I2C_MASTER_SDA 21
#define BUZZER_PWM_PIN 23

// ======== UART (SIM 4G GPS) ========
void comm_uart_init() {
  ESP_LOGI(TAG, "Initializing UART...");
  uart_config_t uart_config = {.baud_rate = 115200,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_driver_install(UART_NUM, 1024 * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM, &uart_config);
  uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
  ESP_LOGI(TAG, "UART initialized.");
}

esp_err_t comm_uart_send_command(const char *command) {
  if (!command)
    return ESP_ERR_INVALID_ARG;

  ESP_LOGI(TAG, "Sending command: %s", command);
  uart_write_bytes(UART_NUM, command, strlen(command));
  uart_write_bytes(UART_NUM, "\r\n", 2);
  return ESP_OK;
}

esp_err_t comm_uart_read_response(char *response, int len) {
  if (!response || len <= 0)
    return ESP_ERR_INVALID_ARG;

  int read_bytes = uart_read_bytes(UART_NUM, (uint8_t *)response, len - 1,
                                   100 / portTICK_PERIOD_MS);
  if (read_bytes <= 0) {
    ESP_LOGW(TAG, "No response received");
    return ESP_FAIL;
  }

  response[read_bytes] = '\0';
  ESP_LOGI(TAG, "Received response: %s", response);
  return ESP_OK;
}

// ======== I2C (MPU6050) ========
void comm_i2c_init() {
  ESP_LOGI(TAG, "Initializing I2C...");
  i2c_config_t conf = {.mode = I2C_MODE_MASTER,
                       .sda_io_num = I2C_MASTER_SDA,
                       .sda_pullup_en = GPIO_PULLUP_ENABLE,
                       .scl_io_num = I2C_MASTER_SCL,
                       .scl_pullup_en = GPIO_PULLUP_ENABLE,
                       .master.clk_speed = 100000};
  i2c_param_config(I2C_NUM_0, &conf);
  i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
  ESP_LOGI(TAG, "I2C initialized.");
}

esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
  uint8_t write_buf[2] = {reg_addr, data};
  return i2c_master_write_to_device(I2C_NUM_0, device_addr, write_buf,
                                    sizeof(write_buf),
                                    100 / portTICK_PERIOD_MS);
}

esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data) {
  return i2c_master_write_read_device(I2C_NUM_0, device_addr, &reg_addr, 1,
                                      data, 1, 100 / portTICK_PERIOD_MS);
}

// ======== PWM (Buzzer) ========
void comm_pwm_init(int frequency) {
  ESP_LOGI(TAG, "Initializing PWM for buzzer...");
  ledc_timer_config_t timer_conf = {.speed_mode = LEDC_HIGH_SPEED_MODE,
                                    .timer_num = LEDC_TIMER_0,
                                    .duty_resolution = LEDC_TIMER_10_BIT,
                                    .freq_hz = frequency};
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t channel_conf = {.gpio_num = BUZZER_PWM_PIN,
                                        .speed_mode = LEDC_HIGH_SPEED_MODE,
                                        .channel = LEDC_CHANNEL_0,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = LEDC_TIMER_0,
                                        .duty = 0};
  ledc_channel_config(&channel_conf);
  ESP_LOGI(TAG, "PWM initialized.");
}

void comm_pwm_set_duty_cycle(int duty_cycle) {
  ESP_LOGI(TAG, "Setting buzzer duty cycle to %d", duty_cycle);
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

void comm_pwm_stop() {
  ESP_LOGI(TAG, "Stopping buzzer...");
  ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}
