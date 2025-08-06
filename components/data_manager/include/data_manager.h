#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "data_manager_types.h"

/**
 * @brief Khởi tạo module quản lý dữ liệu.
 *
 * Hàm này tạo mutex bảo vệ dữ liệu và khởi tạo trạng thái ban đầu của thiết bị.
 * @return esp_err_t ESP_OK nếu khởi tạo thành công.
 */
esp_err_t data_manager_init(void);

/**
 * @brief Giải phóng tài nguyên của module quản lý dữ liệu.
 */
void data_manager_deinit(void);

/**
 * @brief Lấy bản sao dữ liệu hiện tại của thiết bị.
 *
 * Hàm này đảm bảo an toàn luồng và trả về một bản sao dữ liệu.
 * @param[out] state Con trỏ tới cấu trúc device_state_t để lưu bản sao dữ liệu.
 * @return esp_err_t ESP_OK nếu thành công.
 */
esp_err_t data_manager_get_device_state(device_state_t *state);

/**
 * @brief Lấy trạng thái phát hiện ngã hiện tại.
 */
bool data_manager_get_fall_status(void);

/**
 * @brief Lấy tọa độ GPS hiện tại.
 */
void data_manager_get_gps_location(double *latitude, double *longitude);

/**
 * @brief Lấy trạng thái kết nối Wi-Fi hiện tại.
 */
bool data_manager_get_wifi_status(void);

/**
 * @brief Lấy trạng thái kết nối MQTT hiện tại.
 */
bool data_manager_get_mqtt_status(void);

/**
 * @brief Get the device ID.
 * @param id_buffer Buffer to copy the device ID into.
 * @param buffer_size Size of the buffer.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if buffer is NULL or size is 0.
 */
esp_err_t data_manager_get_device_id(char* id_buffer, size_t buffer_size);

/**
 * @brief Cập nhật trạng thái phát hiện ngã.
 *
 * Hàm này cũng sẽ phát ra một sự kiện `EVENT_FALL_DETECTED` nếu trạng thái thay đổi.
 * @param state Trạng thái ngã mới.
 */
esp_err_t data_manager_set_fall_status(bool state);

/**
 * @brief Cập nhật tọa độ GPS.
 * @param latitude Vĩ độ mới.
 * @param longitude Kinh độ mới.
 */
esp_err_t data_manager_set_gps_location(double latitude, double longitude);

/**
 * @brief Cập nhật trạng thái kết nối Wi-Fi.
 * @param connected Trạng thái kết nối mới.
 */
esp_err_t data_manager_set_wifi_status(bool connected);

/**
 * @brief Cập nhật trạng thái kết nối MQTT.
 * @param connected Trạng thái kết nối mới.
 */
esp_err_t data_manager_set_mqtt_status(bool connected);

/**
 * @brief Cập nhật trạng thái đăng ký SIM.
 * @param registered Trạng thái đăng ký mới.
 */
esp_err_t data_manager_set_sim_status(bool registered);

/**
 * @brief Cập nhật ID thiết bị.
 * @param id Con trỏ tới chuỗi ID thiết bị.
 */
esp_err_t data_manager_set_device_id(const char* id);


#ifdef __cplusplus
}
#endif

#endif // _DATA_MANAGER_H_
