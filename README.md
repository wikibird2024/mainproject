### 📂 Cấu trúc Dự án (ESP32 Fall Detection)

- **mainproject/** (Thư mục gốc)
  - `CMakeLists.txt` (Tệp build gốc)
  - `sdkconfig` / `sdkconfig.old` (Cấu hình toàn dự án)
  - `readme.txt` (Tài liệu dự án chính)
  - `dependencies.lock` (Khóa phụ thuộc, ESP-IDF v5+)
  
- **main/** (Entry-point của firmware)
  - `main.c` (Khởi tạo hệ thống, các task chính)
  - `CMakeLists.txt`
  - `idf_component.yml` (Khai báo phụ thuộc ngoài)

- **components/** (Các module riêng biệt)
  - **buzzer/** (Cảnh báo bằng âm thanh)
  - **comm/** (Giao tiếp UART dành cho SIM)
  - **debugs/** (In log, kiểm tra hệ thống)
  - **fall_logic/** (Xử lý logic phát hiện té ngã)
  - **led_indicator/** (Điều khiển LED báo trạng thái)
  - **mpu6050/** (Đọc cảm biến gia tốc)
  - **mqtt_client/** (Gửi JSON đến MQTT Broker)
  - **sim4g_gps/** (SIM EC800K: GPS + SMS)
  - **wifi_connect/** (Kết nối Wi-Fi)
  - `bash.sh` (Script hỗ trợ - **Nên đưa vào thư mục `/tools`**)
ESP32 Fall Detection System
---------------------------

Modular ESP-IDF project to detect falls using MPU6050, alert via buzzer, send GPS via 4G SIM (EC800K), and publish MQTT in JSON format.

Components:
- wifi_connect: WiFi manager
- mqtt_client: JSON publisher
- sim4g_gps: 4G GPS + SMS module
- mpu6050: Motion sensor
- buzzer, led_indicator: Alert system
- fall_logic: Core detection
- comm, debugs: UART + Logging

Main Flow:
1. Connect WiFi
2. Connect MQTT broker
3. Monitor MPU6050
4. On fall: buzz, send SMS with GPS, and publish MQTT JSON

Tested with ESP-IDF v5.x

Author: wikibird | July 2025
