#ifndef _DATA_MANAGER_H_
#define _DATA_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "data_types.h"
#include "esp_err.h"

/**
 * @brief Khởi tạo module quản lý dữ liệu.
 * @note  Hàm này cần được gọi trước khi sử dụng các API khác.
 * @return esp_err_t ESP_OK nếu khởi tạo thành công, ngược lại là mã lỗi.
 */
esp_err_t data_manager_init(void);

/**
 * @brief Lấy bản sao dữ liệu hiện tại của thiết bị.
 * @param[out] data Con trỏ tới cấu trúc device_data_t để lưu bản sao dữ liệu.
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t data_manager_get_data(device_data_t *data);

/**
 * @brief Cập nhật trạng thái phát hiện ngã.
 * @param[in] state Trạng thái mới (true: đã ngã, false: bình thường).
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t data_manager_set_fall_status(bool state);

/**
 * @brief Cập nhật tọa độ GPS.
 * @param[in] latitude Vĩ độ mới.
 * @param[in] longitude Kinh độ mới.
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t data_manager_set_gps_location(double latitude, double longitude);

/**
 * @brief Cập nhật ID thiết bị.
 * @param[in] device_id Con trỏ tới chuỗi ID thiết bị.
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t data_manager_set_device_id(const char *device_id);

/**
 * @brief Cập nhật dấu thời gian hiện tại.
 * @note  Hàm này tự động tạo dấu thời gian hiện tại.
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t data_manager_update_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif // _DATA_MANAGER_H_
