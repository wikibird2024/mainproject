#ifndef _DATA_MANAGER_TYPES_H_
#define _DATA_MANAGER_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Cấu trúc dữ liệu chứa tất cả trạng thái của thiết bị.
 *
 * Đây là nguồn dữ liệu duy nhất (Single Source of Truth) cho toàn bộ hệ thống.
 * Mọi module đều lấy dữ liệu từ đây để đảm bảo tính nhất quán.
 */
typedef struct {
    // Thông tin cơ bản
    char device_id[32];
    uint32_t timestamp_ms;

    // Trạng thái cảm biến
    bool fall_detected;

    // Trạng thái kết nối
    bool wifi_connected;
    bool mqtt_connected;
    bool sim_registered;

    // Dữ liệu GPS
    double latitude;
    double longitude;

    // Thêm các trường dữ liệu khác nếu cần
} device_state_t;

#ifdef __cplusplus
}
#endif

#endif // _DATA_MANAGER_TYPES_H_
