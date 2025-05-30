// sim4g_gps.c
#include "sim4g_gps.h"
#include "comm.h"
#include "debugs.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// -----------------------------------------------------------------------------
// Định nghĩa cấu hình
// -----------------------------------------------------------------------------
#define SO_DIEN_THOAI_MAC_DINH        "+84901234567"
#define THOI_GIAN_COLD_START_MS       60000
#define THOI_GIAN_NORMAL_MS            10000
#define THOI_GIAN_CHECK_MANG_MS       5000
#define DO_MẠNH_TIN_HIEU_TOI_THIEU    5
#define SO_LAN_THU_SMS_TOI_DA          3
#define KICH_THUOC_BUFFER_PHAN_HOI     256

// -----------------------------------------------------------------------------
// Biến tĩnh nội bộ
// -----------------------------------------------------------------------------
static SemaphoreHandle_t mutex_gps = NULL;
static char so_dien_thoai[16] = SO_DIEN_THOAI_MAC_DINH;
static sim4g_gps_data_t vi_tri_cuoi_cung = {0};
static bool gps_cold_start = true;

// -----------------------------------------------------------------------------
// Hàm hỗ trợ nội bộ
// -----------------------------------------------------------------------------

/**
 * @brief Kiểm tra định dạng số điện thoại hợp lệ.
 *        Số hợp lệ bắt đầu bằng '+' và có độ dài từ 11 đến 15 ký tự.
 * 
 * @param so_dien_thoai Chuỗi số điện thoại
 * @return int 1 nếu hợp lệ, 0 nếu không
 */
static int kiem_tra_so_dien_thoai(const char *so_dien_thoai)
{
    if (so_dien_thoai == NULL) return 0;
    int do_dai = strlen(so_dien_thoai);
    return (so_dien_thoai[0] == '+' && do_dai >= 11 && do_dai < 16);
}

/**
 * @brief Kiểm tra trạng thái mạng và cường độ tín hiệu
 * 
 * @return int 1 nếu đã đăng ký mạng và tín hiệu đủ mạnh, 0 nếu không
 */
static int kiem_tra_trang_thai_mang(void)
{
    char phan_hoi[64] = {0};
    int rssi = 0, che_do = 0, trang_thai = 0;
    int da_dang_ky = 0;

    if (comm_uart_send_command("AT+CREG?", phan_hoi, sizeof(phan_hoi)) == COMM_SUCCESS) {
        if (sscanf(phan_hoi, "+CREG: %d,%d", &che_do, &trang_thai) == 2) {
            da_dang_ky = (trang_thai == 1 || trang_thai == 5);
        }
    }

    memset(phan_hoi, 0, sizeof(phan_hoi));
    if (comm_uart_send_command("AT+CSQ", phan_hoi, sizeof(phan_hoi)) == COMM_SUCCESS) {
        sscanf(phan_hoi, "+CSQ: %d", &rssi);
    }

    return (da_dang_ky && (rssi >= DO_MẠNH_TIN_HIEU_TOI_THIEU && rssi != 99));
}

/**
 * @brief Cấu hình chế độ SMS text mode (AT+CMGF=1)
 * 
 * @return int 1 thành công, 0 thất bại
 */
static int cau_hinh_sms_text_mode(void)
{
    char phan_hoi[64] = {0};
    if (comm_uart_send_command("AT+CMGF=1", phan_hoi, sizeof(phan_hoi)) != COMM_SUCCESS) {
        ERROR("Không gửi được lệnh AT+CMGF=1");
        return 0;
    }
    if (strstr(phan_hoi, "OK") == NULL) {
        ERROR("Không nhận được xác nhận SMS text mode");
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
    return 1;
}

/**
 * @brief Đặt số điện thoại nhận SMS (AT+CMGS)
 * 
 * @param so_dien_thoai Số điện thoại người nhận
 * @return int 1 thành công, 0 thất bại
 */
static int dat_so_nhan_sms(const char *so_dien_thoai)
{
    char lenh[64];
    char phan_hoi[64] = {0};

    snprintf(lenh, sizeof(lenh), "AT+CMGS=\"%s\"", so_dien_thoai);
    if (comm_uart_send_command(lenh, phan_hoi, sizeof(phan_hoi)) != COMM_SUCCESS) {
        ERROR("Không đặt được số nhận SMS");
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(300));
    return 1;
}

/**
 * @brief Gửi nội dung SMS và kết thúc bằng Ctrl+Z
 * 
 * @param noi_dung_sms Nội dung tin nhắn
 * @return int 1 thành công, 0 thất bại
 */
static int gui_noi_dung_sms(const char *noi_dung_sms)
{
    char phan_hoi[64] = {0};

    if (comm_uart_send_command(noi_dung_sms, NULL, 0) != COMM_SUCCESS) {
        ERROR("Không gửi được nội dung SMS");
        return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    // Gửi Ctrl+Z (ký tự 26) để kết thúc tin nhắn
    if (comm_uart_send_command("\x1A", phan_hoi, sizeof(phan_hoi)) != COMM_SUCCESS) {
        ERROR("Không gửi được ký tự Ctrl+Z");
        return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(1200));

    if (strstr(phan_hoi, "+CMGS:") || strstr(phan_hoi, "OK")) {
        return 1;
    }

    ERROR("Gửi SMS thất bại: không nhận được phản hồi xác nhận");
    return 0;
}

/**
 * @brief Hàm nội bộ gửi SMS cảnh báo té ngã với dữ liệu GPS
 * 
 * @param vi_tri Con trỏ tới cấu trúc dữ liệu vị trí GPS
 * @return int 1 thành công, 0 thất bại
 */
static int gui_sms_noi_bo(const sim4g_gps_data_t *vi_tri)
{
    char sms[256];

    if (!cau_hinh_sms_text_mode()) return 0;
    if (!dat_so_nhan_sms(so_dien_thoai)) return 0;

    snprintf(sms, sizeof(sms),
        "CANH BAO: Nga!\nVi tri: %s,%s\nTime: %s\nGoogle: https://maps.google.com/?q=%s,%s",
        vi_tri->latitude, vi_tri->longitude, vi_tri->timestamp,
        vi_tri->latitude, vi_tri->longitude);

    if (!gui_noi_dung_sms(sms)) return 0;
    return 1;
}

/**
 * @brief Task FreeRTOS gửi SMS cảnh báo té ngã không đồng bộ
 * 
 * @param param Con trỏ tới vùng nhớ chứa dữ liệu GPS (được cấp phát động)
 */
static void task_gui_sms(void *param)
{
    sim4g_gps_data_t *vi_tri = (sim4g_gps_data_t *)param;
    if (vi_tri == NULL) {
        vTaskDelete(NULL);
        return;
    }

    uint32_t thoi_gian_bat_dau = xTaskGetTickCount();
    int mang_san_sang = 0;

    // Đợi mạng sẵn sàng trong khoảng thời gian giới hạn
    while ((xTaskGetTickCount() - thoi_gian_bat_dau) < pdMS_TO_TICKS(THOI_GIAN_CHECK_MANG_MS)) {
        if (kiem_tra_trang_thai_mang()) {
            mang_san_sang = 1;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(800));
    }

    if (mang_san_sang) {
        for (int i = 0; i < SO_LAN_THU_SMS_TOI_DA; i++) {
            if (gui_sms_noi_bo(vi_tri)) {
                INFO("Gửi SMS thành công (lần %d)", i + 1);
                break;
            }
            if (i < SO_LAN_THU_SMS_TOI_DA - 1) {
                vTaskDelay(pdMS_TO_TICKS(1200));
            }
        }
    } else {
        ERROR("Mạng không khả dụng để gửi SMS");
    }

    vPortFree(vi_tri);
    vTaskDelete(NULL);
}

// -----------------------------------------------------------------------------
// Các hàm API công khai
// -----------------------------------------------------------------------------

/**
 * @brief Khởi tạo GPS và các biến cần thiết
 */
void sim4g_gps_init(void)
{
    char phan_hoi[64] = {0};

    if (mutex_gps == NULL) {
        mutex_gps = xSemaphoreCreateMutex();
        if (mutex_gps == NULL) {
            ERROR("Tạo mutex GPS thất bại");
            return;
        }
    }

    comm_uart_send_command("AT+QGPSCFG=\"autogps\",1", NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    INFO("Khởi động GPS theo kiểu %s start", gps_cold_start ? "COLD" : "HOT");

    if (comm_uart_send_command("AT+QGPS=1", phan_hoi, sizeof(phan_hoi)) == COMM_SUCCESS) {
        if (strstr(phan_hoi, "OK") != NULL) {
            gps_cold_start = false;
            INFO("GPS khởi tạo thành công");
            return;
        }
    }

    ERROR("Khởi tạo GPS thất bại");
}

/**
 * @brief Cập nhật số điện thoại nhận SMS cảnh báo
 * 
 * @param so_moi Số điện thoại mới (dưới dạng +84...)
 */
void sim4g_gps_set_phone_number(const char *so_moi)
{
    if (!kiem_tra_so_dien_thoai(so_moi)) {
        ERROR("Định dạng số điện thoại không hợp lệ");
        return;
    }
    strncpy(so_dien_thoai, so_moi, sizeof(so_dien_thoai) - 1);
    so_dien_thoai[sizeof(so_dien_thoai) - 1] = '\0';
    INFO("Đã cập nhật số điện thoại nhận SMS: %s", so_dien_thoai);
}

/**
 * @brief Lấy vị trí GPS hiện tại
 * 
 * @return sim4g_gps_data_t Dữ liệu vị trí GPS, valid=1 nếu lấy thành công
 */
sim4g_gps_data_t sim4g_gps_get_location(void)
{
    char phan_hoi[KICH_THUOC_BUFFER_PHAN_HOI] = {0};
    sim4g_gps_data_t vi_tri = {0};
    vi_tri.valid = 0;

    if (mutex_gps == NULL) {
        ERROR("Mutex GPS chưa khởi tạo");
        return vi_tri;
    }

    uint32_t timeout = gps_cold_start ? THOI_GIAN_COLD_START_MS : THOI_GIAN_NORMAL_MS;
    uint32_t thoi_gian_bat_dau = xTaskGetTickCount();

    xSemaphoreTake(mutex_gps, portMAX_DELAY);

    while ((xTaskGetTickCount() - thoi_gian_bat_dau) < pdMS_TO_TICKS(timeout)) {
        if (comm_uart_send_command("AT+QGPSLOC?", phan_hoi, sizeof(phan_hoi)) == COMM_SUCCESS) {
            // Dữ liệu trả về dạng: +QGPSLOC: 20250530091500.000,21.028511,N,105.854132,E,120.0,0.0,0.0,1,7,1.0,1.0,1.0,,,
            // Cần phân tích để lấy tọa độ và thời gian
            if (strstr(phan_hoi, "+QGPSLOC:") != NULL) {
                // Phân tích chuỗi tọa độ và thời gian
                sscanf(phan_hoi, "+QGPSLOC: %15[^,],%10[^,],%1[^,],%11[^,],%1[^,],",
                    vi_tri.timestamp, vi_tri.latitude, vi_tri.lat_dir,
                    vi_tri.longitude, vi_tri.lon_dir);

                vi_tri.valid = 1;

                // Lưu vị trí cuối cùng
                memcpy(&vi_tri_cuoi_cung, &vi_tri, sizeof(sim4g_gps_data_t));
                gps_cold_start = false;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (!vi_tri.valid) {
        // Trả vị trí cũ nếu chưa có dữ liệu mới
        vi_tri = vi_tri_cuoi_cung;
    }

    xSemaphoreGive(mutex_gps);
    return vi_tri;
}

/**
 * @brief Gửi SMS cảnh báo té ngã với vị trí GPS không đồng bộ
 * 
 * @param vi_tri Dữ liệu vị trí GPS
 */
void sim4g_gps_send_fall_alert_async(sim4g_gps_data_t *vi_tri)
{
    if (vi_tri == NULL || !vi_tri->valid) {
        ERROR("Dữ liệu GPS không hợp lệ, không gửi SMS");
        return;
    }

    sim4g_gps_data_t *vi_tri_copy = pvPortMalloc(sizeof(sim4g_gps_data_t));
    if (vi_tri_copy == NULL) {
        ERROR("Không cấp phát được bộ nhớ cho dữ liệu SMS");
        return;
    }
    memcpy(vi_tri_copy, vi_tri, sizeof(sim4g_gps_data_t));

    if (xTaskCreate(task_gui_sms, "task_gui_sms", 4096, vi_tri_copy, 5, NULL) != pdPASS) {
        ERROR("Tạo task gửi SMS thất bại");
        vPortFree(vi_tri_copy);
    }
}
