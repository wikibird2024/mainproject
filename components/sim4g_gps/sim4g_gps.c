Kiểm tra đoạn code sau .#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define TAG "SIM4G_GPS"

// -----------------------------------------------------------------------------
// Định nghĩa cấu hình
// -----------------------------------------------------------------------------
#define DEFAULT_PHONE_NUMBER        "+84901234567"
#define COLD_START_TIME_MS          60000
#define NORMAL_OPERATION_TIME_MS    10000
#define NETWORK_CHECK_TIME_MS       5000
#define MIN_SIGNAL_STRENGTH         5
#define MAX_SMS_RETRY_COUNT         3
#define RESPONSE_BUFFER_SIZE        256
#define SIM4G_GPS_RESPONSE_BUFFER_SIZE 256
#define RETRY_DELAY_MS              500

// -----------------------------------------------------------------------------
// Biến tĩnh nội bộ
// -----------------------------------------------------------------------------
static SemaphoreHandle_t gps_mutex = NULL;
static char phone_number[16] = DEFAULT_PHONE_NUMBER;
static sim4g_gps_data_t last_known_location = {0};
static bool is_gps_cold_start = true;

// -----------------------------------------------------------------------------
// Hàm hỗ trợ nội bộ
// -----------------------------------------------------------------------------

/**

@brief Kiểm tra định dạng số điện thoại hợp lệ
*/
static bool validate_phone_number(const char *number)
{
if (number == NULL) return false;
size_t length = strlen(number);
return (number[0] == '+' && length >= 11 && length < 16);
}


/**

@brief Kiểm tra trạng thái mạng và cường độ tín hiệu
*/
static bool check_network_status(void)
{
char response[64] = {0};
int rssi = 0, mode = 0, status = 0;
bool is_registered = false;

comm_result_t res = comm_uart_send_command("AT+CREG?", response, sizeof(response));
if (res != COMM_SUCCESS) {
ESP_LOGE(TAG, "Lỗi kiểm tra đăng ký mạng: %d", res);
return false;
}

if (sscanf(response, "+CREG: %d,%d", &mode, &status) == 2) {
is_registered = (status == 1 || status == 5);
}

memset(response, 0, sizeof(response));
res = comm_uart_send_command("AT+CSQ", response, sizeof(response));
if (res != COMM_SUCCESS) {
ESP_LOGE(TAG, "Lỗi kiểm tra cường độ tín hiệu: %d", res);
return false;
}

if (sscanf(response, "+CSQ: %d", &rssi) != 1) {
return false;
}

return (is_registered && (rssi >= MIN_SIGNAL_STRENGTH && rssi != 99));
}


/**

@brief Cấu hình chế độ SMS text mode
*/
static bool configure_sms_text_mode(void)
{
for (int i = 0; i < MAX_SMS_RETRY_COUNT; i++) {
comm_result_t res = comm_uart_send_command("AT+CMGF=1", NULL, 0);
if (res == COMM_SUCCESS) {
vTaskDelay(pdMS_TO_TICKS(200));
return true;
}
vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
}
ESP_LOGE(TAG, "Không thể cấu hình SMS text mode");
return false;
}


/**

@brief Đặt số điện thoại nhận SMS
*/
static bool set_sms_recipient(const char *number)
{
char command[64];
snprintf(command, sizeof(command), "AT+CMGS="%s"", number);

for (int i = 0; i < MAX_SMS_RETRY_COUNT; i++) {
comm_result_t res = comm_uart_send_command(command, NULL, 0);
if (res == COMM_SUCCESS) {
vTaskDelay(pdMS_TO_TICKS(300));
return true;
}
vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
}
ESP_LOGE(TAG, "Không thể đặt số nhận SMS");
return false;
}


/**

@brief Gửi nội dung SMS và kết thúc bằng Ctrl+Z
*/
static bool send_sms_content(const char *content)
{
comm_result_t res = comm_uart_send_command(content, NULL, 0);
if (res != COMM_SUCCESS) {
ESP_LOGE(TAG, "Không gửi được nội dung SMS");
return false;
}

vTaskDelay(pdMS_TO_TICKS(200));

// Gửi Ctrl+Z (ký tự 26) để kết thúc tin nhắn
char response[64] = {0};
res = comm_uart_send_command("\x1A", response, sizeof(response));
if (res != COMM_SUCCESS) {
ESP_LOGE(TAG, "Không gửi được ký tự Ctrl+Z");
return false;
}

vTaskDelay(pdMS_TO_TICKS(1200));

if (strstr(response, "+CMGS:") == NULL && strstr(response, "OK") == NULL) {
ESP_LOGE(TAG, "Gửi SMS thất bại: không nhận được phản hồi xác nhận");
return false;
}

return true;
}


/**

@brief Hàm nội bộ gửi SMS cảnh báo
*/
static bool send_alert_sms(const sim4g_gps_data_t *location)
{
if (location == NULL || !location->valid) {
ESP_LOGE(TAG, "Dữ liệu GPS không hợp lệ");
return false;
}

char sms[256];
snprintf(sms, sizeof(sms),
"CANH BAO: Nga!\nVi tri: %s,%s\nTime: %s\nGoogle: https://maps.google.com/?q=%s,%s",
location->latitude, location->longitude, location->timestamp,
location->latitude, location->longitude);

if (!configure_sms_text_mode()) return false;
if (!set_sms_recipient(phone_number)) return false;
if (!send_sms_content(sms)) return false;

return true;
}


/**

@brief Task FreeRTOS gửi SMS cảnh báo không đồng bộ
*/
static void sms_sending_task(void *param)
{
sim4g_gps_data_t *location = (sim4g_gps_data_t *)param;
if (location == NULL) {
vTaskDelete(NULL);
return;
}

uint32_t start_time = xTaskGetTickCount();
bool network_ready = false;

// Đợi mạng sẵn sàng trong khoảng thời gian giới hạn
while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(NETWORK_CHECK_TIME_MS)) {
if (check_network_status()) {
network_ready = true;
break;
}
vTaskDelay(pdMS_TO_TICKS(800));
}

if (network_ready) {
for (int i = 0; i < MAX_SMS_RETRY_COUNT; i++) {
if (send_alert_sms(location)) {
ESP_LOGI(TAG, "Gửi SMS thành công (lần %d)", i + 1);
break;
}
if (i < MAX_SMS_RETRY_COUNT - 1) {
vTaskDelay(pdMS_TO_TICKS(1200));
}
}
} else {
ESP_LOGE(TAG, "Mạng không khả dụng để gửi SMS");
}

vPortFree(location);
vTaskDelete(NULL);
}


// -----------------------------------------------------------------------------
// Các hàm API công khai
// -----------------------------------------------------------------------------

void sim4g_gps_init(void)
{
if (gps_mutex == NULL) {
gps_mutex = xSemaphoreCreateMutex();
if (gps_mutex == NULL) {
ESP_LOGE(TAG, "Tạo mutex GPS thất bại");
return;
}
}

comm_uart_init();  
comm_uart_send_command("AT+QGPSCFG=\"autogps\",1", NULL, 0);  
vTaskDelay(pdMS_TO_TICKS(200));  

ESP_LOGI(TAG, "Khởi động GPS theo kiểu %s start", is_gps_cold_start ? "COLD" : "HOT");  

char response[64] = {0};  
comm_result_t res = comm_uart_send_command("AT+QGPS=1", response, sizeof(response));  
if (res == COMM_SUCCESS && strstr(response, "OK") != NULL) {  
    is_gps_cold_start = false;  
    ESP_LOGI(TAG, "GPS khởi tạo thành công");  
} else {  
    ESP_LOGE(TAG, "Khởi tạo GPS thất bại, lỗi: %d", res);  
}

}

void sim4g_gps_set_phone_number(const char *new_number)
{
if (!validate_phone_number(new_number)) {
ESP_LOGE(TAG, "Định dạng số điện thoại không hợp lệ");
return;
}
strncpy(phone_number, new_number, sizeof(phone_number) - 1);
phone_number[sizeof(phone_number) - 1] = '\0';
ESP_LOGI(TAG, "Đã cập nhật số điện thoại nhận SMS: %s", phone_number);
}

sim4g_gps_data_t sim4g_gps_get_location(void)
{
char response[RESPONSE_BUFFER_SIZE] = {0};
sim4g_gps_data_t location = {0};
location.valid = 0;

if (gps_mutex == NULL) {  
    ESP_LOGE(TAG, "Mutex GPS chưa khởi tạo");  
    return location;  
}  

uint32_t timeout = is_gps_cold_start ? COLD_START_TIME_MS : NORMAL_OPERATION_TIME_MS;  
uint32_t start_time = xTaskGetTickCount();  

if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(timeout)) != pdTRUE) {  
    ESP_LOGE(TAG, "Timeout khi chờ mutex GPS");  
    return last_known_location;  
}  

while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(timeout)) {  
    comm_result_t res = comm_uart_send_command("AT+QGPSLOC?", response, sizeof(response));  
      
    if (res == COMM_SUCCESS && strstr(response, "+QGPSLOC:") != NULL) {  
        // Phân tích chuỗi tọa độ và thời gian  
        if (sscanf(response, "+QGPSLOC: %15[^,],%10[^,],%1[^,],%11[^,],%1[^,],",  
            location.timestamp, location.latitude, location.lat_dir,  
            location.longitude, location.lon_dir) == 5) {  
              
            location.valid = 1;  
            memcpy(&last_known_location, &location, sizeof(sim4g_gps_data_t));  
            is_gps_cold_start = false;  
            break;  
        }  
    }  
    vTaskDelay(pdMS_TO_TICKS(1000));  
}  

xSemaphoreGive(gps_mutex);  

if (!location.valid) {  
    location = last_known_location;  
}  

return location;

}

void sim4g_gps_send_fall_alert_async(const sim4g_gps_data_t *location)
{
if (location == NULL || !location->valid) {
ESP_LOGE(TAG, "Dữ liệu GPS không hợp lệ, không gửi SMS");
return;
}

sim4g_gps_data_t *location_copy = pvPortMalloc(sizeof(sim4g_gps_data_t));  
if (location_copy == NULL) {  
    ESP_LOGE(TAG, "Không cấp phát được bộ nhớ cho dữ liệu SMS");  
    return;  
}  
memcpy(location_copy, location, sizeof(sim4g_gps_data_t));  

if (xTaskCreate(sms_sending_task, "sms_sending_task", 4096, location_copy, 5, NULL) != pdPASS) {  
    ESP_LOGE(TAG, "Tạo task gửi SMS thất bại");  
    vPortFree(location_copy);  
}

}

