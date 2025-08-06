#ifndef _USER_MQTT_H_
#define _USER_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @brief Khởi tạo và kết nối client MQTT.
 *
 * @param broker_uri URI của MQTT broker (ví dụ: "mqtt://broker.hivemq.com").
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t user_mqtt_init(const char *broker_uri);

/**
 * @brief Tự động lấy dữ liệu mới nhất từ Data Manager và publish lên MQTT broker.
 * @note  Hàm này sẽ gọi json_wrapper để tạo payload.
 *
 * @param topic Chủ đề (topic) MQTT để publish.
 * @param qos Quality of Service của thông điệp.
 * @param retain Flag để giữ lại thông điệp trên broker.
 * @return esp_err_t ESP_OK nếu thành công, ngược lại là mã lỗi.
 */
esp_err_t user_mqtt_publish_current_data(const char *topic, int qos, int retain);

#ifdef __cplusplus
}
#endif

#endif // _USER_MQTT_H_
