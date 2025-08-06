#ifndef _JSON_WRAPPER_H_
#define _JSON_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Lấy dữ liệu từ Data Manager và tạo một chuỗi JSON.
 * @note  Chuỗi JSON trả về cần được giải phóng bộ nhớ bởi người gọi (sử dụng free()).
 * @return char* Con trỏ đến chuỗi JSON, hoặc NULL nếu có lỗi.
 */
char *json_wrapper_create_payload(void);

#ifdef __cplusplus
}
#endif

#endif // _JSON_WRAPPER_H_
