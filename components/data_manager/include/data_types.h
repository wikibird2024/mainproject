#ifndef _DATA_TYPES_H_
#define _DATA_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Cấu trúc dữ liệu đại diện cho trạng thái và vị trí của thiết bị.
 * @note  Cấu trúc này được sử dụng nội bộ bởi data_manager.
 */
typedef struct {
    char device_id[32];
    bool fall_detected;
    char timestamp[20];
    double latitude;
    double longitude;
} device_data_t;

#ifdef __cplusplus
}
#endif

#endif // _DATA_TYPES_H_
