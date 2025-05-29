#pragma once

/**
 * @file sim4g_gps.h
 * @brief API điều khiển module SIM 4G GPS để lấy vị trí và gửi cảnh báo SMS.
 *
 * Module cung cấp các hàm khởi tạo GPS, lấy dữ liệu GPS, thiết lập số điện thoại
 * và gửi tin nhắn cảnh báo khi phát hiện sự cố.
 */

#include <stdbool.h>
#include <stddef.h>  // Nếu có dùng size_t, ở đây chưa cần
#include <stdint.h>  // Nếu có dùng kiểu số chuẩn, ở đây chưa cần

#ifdef __cplusplus
extern "C" {
#endif

/// Kích thước tối đa cho chuỗi chứa dữ liệu GPS như latitude, longitude, timestamp
#define SIM4G_GPS_STRING_MAX_LEN 20
#define SIM4G_GPS_PHONE_MAX_LEN  16

/**
 * @brief Struct dữ liệu vị trí GPS lấy được từ module.
 *
 * Chứa vĩ độ, kinh độ và timestamp theo chuẩn UTC.
 * Trường `valid` báo hiệu dữ liệu GPS có hợp lệ hay không.
 */
typedef struct {
    char latitude[SIM4G_GPS_STRING_MAX_LEN];   /**< Vĩ độ dạng chuỗi */
    char longitude[SIM4G_GPS_STRING_MAX_LEN];  /**< Kinh độ dạng chuỗi */
    char timestamp[SIM4G_GPS_STRING_MAX_LEN];  /**< Thời gian UTC dạng chuỗi */
    bool valid;                                /**< Flag báo dữ liệu hợp lệ */
} sim4g_gps_data_t;

/**
 * @brief Khởi tạo module GPS, bật GPS trên module SIM 4G.
 *
 * Gửi lệnh AT để cấu hình và bật GPS.
 * Nên gọi hàm này một lần khi khởi động hệ thống.
 */
void sim4g_gps_init(void);

/**
 * @brief Thiết lập số điện thoại nhận tin nhắn cảnh báo.
 *
 * @param[in] phone_number Chuỗi số điện thoại dạng "+84901234567", tối đa 15 ký tự (không tính '\0').
 */
void sim4g_gps_set_phone_number(const char *phone_number);

/**
 * @brief Lấy dữ liệu vị trí GPS hiện tại từ module.
 *
 * @return Dữ liệu GPS với flag `valid` để biết vị trí hợp lệ hay không.
 */
sim4g_gps_data_t sim4g_gps_get_location(void);

/**
 * @brief Gửi tin nhắn SMS cảnh báo té ngã với vị trí GPS hiện tại.
 *
 * Hàm tạo một task riêng để gửi SMS không gây block luồng chính.
 *
 * @param[in] location Con trỏ tới dữ liệu GPS hợp lệ dùng để gửi tin nhắn.
 */
void send_fall_alert_sms(const sim4g_gps_data_t *location);

#ifdef __cplusplus
}
#endif
