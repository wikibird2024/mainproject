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
 * @brief Struct chứa dữ liệu GPS lấy từ module SIM 4G.
 */
typedef struct {
    char latitude[SIM4G_GPS_STRING_MAX_LEN];   ///< Vĩ độ dưới dạng chuỗi ký tự
    char lat_dir[2];                          ///< Hướng vĩ độ ('N' hoặc 'S')
    char longitude[SIM4G_GPS_STRING_MAX_LEN];  ///< Kinh độ dưới dạng chuỗi ký tự  
    char lon_dir[2];                          ///< Hướng kinh độ ('E' hoặc 'W')
    char timestamp[SIM4G_GPS_STRING_MAX_LEN];  ///< Thời gian lấy dữ liệu GPS
    bool valid;                                ///< Trạng thái dữ liệu hợp lệ hay không
} sim4g_gps_data_t;

/**
 * @brief Khởi tạo module SIM 4G GPS (gửi lệnh AT bật GPS).
 */
void sim4g_gps_init(void);

/**
 * @brief Thiết lập số điện thoại nhận cảnh báo SMS.
 * @param phone_number Chuỗi số điện thoại ví dụ "+84901234567"
 */
void sim4g_gps_set_phone_number(const char *phone_number);

/**
 * @brief Lấy dữ liệu vị trí GPS hiện tại.
 * @return Struct chứa tọa độ, thời gian và trạng thái hợp lệ
 */
sim4g_gps_data_t sim4g_gps_get_location(void);

/**
 * @brief Gửi SMS cảnh báo té ngã kèm vị trí GPS, chạy task gửi SMS bất đồng bộ.
 * @param location Con trỏ tới dữ liệu GPS hợp lệ
 */
void sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location);

#ifdef __cplusplus
}
#endif
