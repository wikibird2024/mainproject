#pragma once

/**
 * @file sim4g_gps.h
 * @brief API điều khiển module SIM 4G GPS để lấy vị trí và gửi cảnh báo SMS.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Độ dài tối đa cho chuỗi GPS (vĩ độ, kinh độ, thời gian)
#define SIM4G_GPS_STRING_MAX_LEN  20

/// Độ dài tối đa cho số điện thoại (kể cả ký tự '+', không tính '\0')
#define SIM4G_GPS_PHONE_MAX_LEN   16

/**
 * @brief Mã lỗi trả về từ các hàm API.
 */
typedef enum {
    SIM4G_SUCCESS = 0,                ///< Thành công
    SIM4G_ERROR_MUTEX = -1,           ///< Lỗi mutex
    SIM4G_ERROR_COMM_NOT_READY = -2,  ///< UART chưa sẵn sàng
    SIM4G_ERROR_NETWORK = -3,         ///< Lỗi kết nối mạng
    SIM4G_ERROR_GPS_TIMEOUT = -4,     ///< Hết thời gian chờ GPS
    SIM4G_ERROR_SMS_SEND = -5,        ///< Lỗi gửi SMS
    SIM4G_ERROR_INVALID_DATA = -6,    ///< Dữ liệu không hợp lệ
    SIM4G_ERROR_MEMORY = -7,          ///< Lỗi cấp phát bộ nhớ
    SIM4G_ERROR_INVALID_PARAM = -8    ///< Tham số không hợp lệ
} sim4g_error_t;

/**
 * @brief Struct chứa dữ liệu GPS lấy từ module SIM 4G.
 */
typedef struct {
    char latitude[SIM4G_GPS_STRING_MAX_LEN];   ///< Vĩ độ dưới dạng chuỗi ký tự
    char lat_dir[2];                          ///< Hướng vĩ độ ('N' hoặc 'S')
    char longitude[SIM4G_GPS_STRING_MAX_LEN];  ///< Kinh độ dưới dạng chuỗi ký tự  
    char lon_dir[2];                          ///< Hướng kinh độ ('E' hoặc 'W')
    char timestamp[SIM4G_GPS_STRING_MAX_LEN];  ///< Thời gian lấy dữ liệu GPS
    bool valid;                               ///< Trạng thái dữ liệu hợp lệ hay không
} sim4g_gps_data_t;

/**
 * @brief Callback thông báo trạng thái gửi SMS.
 * @param success True nếu SMS được gửi thành công, false nếu thất bại.
 */
typedef void (*sms_callback_t)(bool success);

/**
 * @brief Khởi tạo module SIM 4G GPS với timeout tùy chỉnh.
 * @param cold_start_ms Thời gian chờ GPS cold start (ms).
 * @param normal_ms Thời gian chờ GPS bình thường (ms).
 * @param network_ms Thời gian chờ kiểm tra mạng (ms).
 */
void sim4g_gps_init_with_timeout(uint32_t cold_start_ms, uint32_t normal_ms, uint32_t network_ms);

/**
 * @brief Khởi tạo module SIM 4G GPS với timeout mặc định.
 */
void sim4g_gps_init(void);

/**
 * @brief Thiết lập số điện thoại nhận cảnh báo SMS.
 * @param phone_number Chuỗi số điện thoại (ví dụ: "+84901234567", tối đa 16 ký tự).
 * @return SIM4G_SUCCESS nếu thành công, hoặc mã lỗi sim4g_error_t nếu thất bại.
 */
sim4g_error_t sim4g_gps_set_phone_number(const char *phone_number);

/**
 * @brief Lấy dữ liệu vị trí GPS hiện tại.
 * @return Struct chứa tọa độ, thời gian và trạng thái hợp lệ.
 */
sim4g_gps_data_t sim4g_gps_get_location(void);

/**
 * @brief Gửi SMS cảnh báo té ngã kèm vị trí GPS, chạy task gửi SMS bất đồng bộ.
 * @param location Con trỏ tới dữ liệu GPS hợp lệ.
 * @param callback Hàm callback được gọi khi gửi SMS hoàn tất.
 * @return SIM4G_SUCCESS nếu task được tạo thành công, hoặc mã lỗi sim4g_error_t nếu thất bại.
 */
sim4g_error_t sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location, sms_callback_t callback);

#ifdef __cplusplus
}
#endif