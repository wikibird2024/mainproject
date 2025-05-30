/**
 * @file comm.h
 * @brief Giao diện cho component comm – quản lý giao tiếp UART, I2C, PWM và GPIO.
 *
 * Cung cấp hàm khởi tạo và sử dụng các giao tiếp ngoại vi như UART (SIM 4G GPS), 
 * I2C (MPU6050), PWM (buzzer/LED), và GPIO (LED, nút nhấn).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mã lỗi trả về của các hàm trong comm component.
 */
typedef enum {
    COMM_SUCCESS = 0,          ///< Thành công
    COMM_ERROR = -1,           ///< Lỗi chung
    COMM_TIMEOUT = -2,         ///< Hết thời gian chờ phản hồi
    COMM_INVALID_PARAM = -3    ///< Tham số không hợp lệ
} comm_result_t;

// ========================== UART (SIM 4G GPS) ==========================

/**
 * @brief Khởi tạo giao tiếp UART với module SIM 4G.
 *
 * Cấu hình các thông số như baudrate, chân TX/RX, và mở UART driver.
 */
void comm_uart_init(void);

/**
 * @brief Gửi lệnh AT tới module SIM 4G và đọc phản hồi từ UART.
 *
 * @param[in]  command       Chuỗi lệnh AT cần gửi (NULL-terminated).
 * @param[out] response_buf  Bộ đệm lưu phản hồi nhận được (có thể NULL nếu không cần).
 * @param[in]  buf_size      Kích thước tối đa của bộ đệm phản hồi.
 * @return comm_result_t     Trạng thái kết quả giao tiếp.
 */
comm_result_t comm_uart_send_command(const char *command, char *response_buf, size_t buf_size);

// ========================== I2C (MPU6050) ==========================

/**
 * @brief Khởi tạo giao tiếp I2C.
 *
 * Cấu hình các chân SDA/SCL, tốc độ I2C và khởi tạo driver I2C.
 */
void comm_i2c_init(void);

/**
 * @brief Ghi một byte tới thiết bị I2C.
 *
 * @param[in] dev   Địa chỉ thiết bị I2C.
 * @param[in] reg   Thanh ghi cần ghi.
 * @param[in] data  Giá trị byte cần ghi.
 * @return esp_err_t ESP_OK nếu thành công, lỗi khác nếu thất bại.
 */
esp_err_t comm_i2c_write(uint8_t dev, uint8_t reg, uint8_t data);

/**
 * @brief Đọc một byte từ thiết bị I2C.
 *
 * @param[in]  dev   Địa chỉ thiết bị I2C.
 * @param[in]  reg   Thanh ghi cần đọc.
 * @param[out] data  Con trỏ lưu giá trị đọc được.
 * @return esp_err_t ESP_OK nếu thành công, lỗi khác nếu thất bại.
 */
esp_err_t comm_i2c_read(uint8_t dev, uint8_t reg, uint8_t *data);

// ========================== PWM (Buzzer/LED) ==========================

/**
 * @brief Khởi tạo PWM trên chân GPIO.
 *
 * @param[in] pin   Số hiệu chân GPIO cần phát PWM.
 * @param[in] freq  Tần số PWM mong muốn (Hz).
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t comm_pwm_init(int pin, int freq);

/**
 * @brief Cài đặt độ rộng xung PWM (duty cycle).
 *
 * @param[in] duty  Giá trị duty cycle (0–1023).
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t comm_pwm_set_duty_cycle(int duty);

/**
 * @brief Dừng phát xung PWM.
 *
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t comm_pwm_stop(void);

// ========================== GPIO (LED & Button) ==========================

/**
 * @brief Khởi tạo GPIO cho LED và nút nhấn.
 *
 * @param[in] led_pin     Số chân GPIO nối với LED.
 * @param[in] button_pin  Số chân GPIO nối với nút nhấn.
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t comm_gpio_init(int led_pin, int button_pin);

/**
 * @brief Thiết lập trạng thái LED.
 *
 * @param[in] state  Trạng thái LED: 0 = tắt, khác 0 = bật.
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t comm_gpio_led_set(int state);

/**
 * @brief Đọc trạng thái nút nhấn.
 *
 * @param[out] pressed  Con trỏ lưu kết quả: true nếu đang nhấn, false nếu không.
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t comm_gpio_button_read(bool *pressed);

#ifdef __cplusplus
}
#endif
