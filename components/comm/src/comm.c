#include "comm.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "COMM";

// Static variables for component state
static bool uart_initialized = false;
static bool i2c_initialized = false;
static bool pwm_initialized = false;
static bool gpio_initialized = false;
static int pwm_pin = -1;

// PWM configuration
#define PWM_TIMER LEDC_TIMER_0
#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_RESOLUTION LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY (255)

/**
 * Initialize all communication interfaces
 */
esp_err_t comm_init_all(void) {
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing all communication interfaces...");

    // Initialize UART with default config from menuconfig
    ret = comm_uart_init(UART_NUM, UART_TX_PIN, UART_RX_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UART: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize I2C
    ret = comm_i2c_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize GPIO
    ret = comm_gpio_init(DEFAULT_LED_GPIO, DEFAULT_BUTTON_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "All communication interfaces initialized successfully");
    return ESP_OK;
}

/**
 * Initialize UART communication
 */
esp_err_t comm_uart_init(int uart_num, int tx_pin, int rx_pin) {
    if (uart_initialized) {
        ESP_LOGW(TAG, "UART already initialized");
        return ESP_OK;
    }

    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(uart_num, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_param_config(uart_num, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        uart_driver_delete(uart_num);
        return ret;
    }

    ret = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        uart_driver_delete(uart_num);
        return ret;
    }

    uart_initialized = true;
    ESP_LOGI(TAG, "UART initialized on port %d (TX: %d, RX: %d)", uart_num, tx_pin, rx_pin);
    return ESP_OK;
}

/**
 * Send command via UART and wait for response
 */
comm_result_t comm_uart_send_command(const char *command, char *response_buf, size_t buf_size, int timeout_ms) {
    if (!uart_initialized) {
        ESP_LOGE(TAG, "UART not initialized");
        return COMM_INVALID_PARAM;
    }

    if (command == NULL || response_buf == NULL || buf_size == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return COMM_INVALID_PARAM;
    }

    // Clear response buffer
    memset(response_buf, 0, buf_size);

    // Send command
    int sent = uart_write_bytes(UART_NUM, command, strlen(command));
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send UART command");
        return COMM_INVALID_PARAM;
    }

    // Wait for response with the provided timeout
    int received = uart_read_bytes(UART_NUM, (uint8_t *)response_buf, buf_size - 1, pdMS_TO_TICKS(timeout_ms));
    if (received < 0) {
        ESP_LOGE(TAG, "UART read error");
        return COMM_INVALID_PARAM;
    } else if (received == 0) {
        ESP_LOGW(TAG, "UART read timeout");
        return COMM_TIMEOUT;
    }

    response_buf[received] = '\0'; // Null terminate
    ESP_LOGD(TAG, "UART command sent: %s, response: %s", command, response_buf);
    return COMM_SUCCESS;
}

/**
 * Initialize I2C master
 */
esp_err_t comm_i2c_init(void) {
    if (i2c_initialized) {
        ESP_LOGW(TAG, "I2C already initialized");
        return ESP_OK;
    }

    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_CLOCK_SPEED,
    };

    esp_err_t ret = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C parameters: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_initialized = true;
    ESP_LOGI(TAG, "I2C initialized on port %d (SDA: %d, SCL: %d)", I2C_MASTER_PORT, I2C_MASTER_SDA, I2C_MASTER_SCL);
    return ESP_OK;
}

/**
 * Write data to I2C device register
 */
esp_err_t comm_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t data) {
    if (!i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "I2C write: addr=0x%02X, reg=0x%02X, data=0x%02X", device_addr, reg_addr, data);
    }

    return ret;
}

/**
 * Write single byte to I2C device register (alias for comm_i2c_write)
 */
esp_err_t comm_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t data) {
    return comm_i2c_write(addr, reg, data);
}

/**
 * Read data from I2C device register
 */
esp_err_t comm_i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *data, size_t size) {
    if (!i2c_initialized) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL || size == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Write register address
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    // Read data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);

    if (size > 1) {
        i2c_master_read(cmd, data, size - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + size - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "I2C read: addr=0x%02X, reg=0x%02X, size=%d", device_addr, reg, size);
    }

    return ret;
}

/**
 * Read single byte from I2C device register
 */
esp_err_t comm_i2c_read_byte(uint8_t addr, uint8_t reg, uint8_t *data) {
    return comm_i2c_read(addr, reg, data, 1);
}

/**
 * Initialize PWM on specified pin
 */
esp_err_t comm_pwm_init(int pin, int freq) {
    if (pwm_initialized) {
        ESP_LOGW(TAG, "PWM already initialized");
        return ESP_OK;
    }

    // Configure timer
    ledc_timer_config_t ledc_timer = {.speed_mode = PWM_MODE,
                                        .timer_num = PWM_TIMER,
                                        .duty_resolution = PWM_RESOLUTION,
                                        .freq_hz = freq,
                                        .clk_cfg = LEDC_AUTO_CLK};

    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PWM timer: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure channel
    ledc_channel_config_t ledc_channel = {.speed_mode = PWM_MODE,
                                        .channel = PWM_CHANNEL,
                                        .timer_sel = PWM_TIMER,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .gpio_num = pin,
                                        .duty = 0,
                                        .hpoint = 0};

    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PWM channel: %s", esp_err_to_name(ret));
        return ret;
    }

    pwm_pin = pin;
    pwm_initialized = true;
    ESP_LOGI(TAG, "PWM initialized on pin %d with frequency %d Hz", pin, freq);
    return ESP_OK;
}

/**
 * Set PWM duty cycle (0-100%)
 */
esp_err_t comm_pwm_set_duty_cycle(int duty) {
    if (!pwm_initialized) {
        ESP_LOGE(TAG, "PWM not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (duty < 0 || duty > 100) {
        ESP_LOGE(TAG, "Invalid duty cycle: %d (must be 0-100)", duty);
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t duty_val = (duty * PWM_MAX_DUTY) / 100;

    esp_err_t ret = ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty_val);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set PWM duty: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = ledc_update_duty(PWM_MODE, PWM_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update PWM duty: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "PWM duty cycle set to %d%% (raw: %lu)", duty, duty_val);
    return ESP_OK;
}

/**
 * Stop PWM output
 */
esp_err_t comm_pwm_stop(void) {
    if (!pwm_initialized) {
        ESP_LOGW(TAG, "PWM not initialized");
        return ESP_OK;
    }

    esp_err_t ret = ledc_stop(PWM_MODE, PWM_CHANNEL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop PWM: %s", esp_err_to_name(ret));
        return ret;
    }

    pwm_initialized = false;
    ESP_LOGI(TAG, "PWM stopped");
    return ESP_OK;
}

/**
 * Initialize GPIO pins for LED and button
 */
esp_err_t comm_gpio_init(int led_pin, int button_pin) {
    if (gpio_initialized) {
        ESP_LOGW(TAG, "GPIO already initialized");
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    // Configure LED pin as output
    gpio_config_t led_config = {.pin_bit_mask = (1ULL << led_pin),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE};

    ret = gpio_config(&led_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure button pin as input with pull-up
    gpio_config_t button_config = {.pin_bit_mask = (1ULL << button_pin),
                                    .mode = GPIO_MODE_INPUT,
                                    .pull_up_en = GPIO_PULLUP_ENABLE,
                                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                    .intr_type = GPIO_INTR_DISABLE};

    ret = gpio_config(&button_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set LED to initial OFF state
    gpio_set_level(led_pin, 0);

    gpio_initialized = true;
    ESP_LOGI(TAG, "GPIO initialized (LED: %d, Button: %d)", led_pin, button_pin);
    return ESP_OK;
}

/**
 * Set LED state (0=OFF, 1=ON)
 */
esp_err_t comm_gpio_led_set(int state) {
    if (!gpio_initialized) {
        ESP_LOGE(TAG, "GPIO not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(DEFAULT_LED_GPIO, state ? 1 : 0);
    ESP_LOGD(TAG, "LED set to %s", state ? "ON" : "OFF");
    return ESP_OK;
}

/**
 * Read button state (true=pressed, false=not pressed)
 */
esp_err_t comm_gpio_button_read(bool *pressed) {
    if (!gpio_initialized) {
        ESP_LOGE(TAG, "GPIO not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (pressed == NULL) {
        ESP_LOGE(TAG, "Invalid parameter");
        return ESP_ERR_INVALID_ARG;
    }

    // Button is active low (pressed = 0, not pressed = 1)
    int level = gpio_get_level(DEFAULT_BUTTON_GPIO);
    *pressed = (level == 0);

    ESP_LOGD(TAG, "Button state: %s", *pressed ? "PRESSED" : "NOT PRESSED");
    return ESP_OK;
}
