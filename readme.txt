# Du an canh bao te nga va dinh vi GPS dung ESP32

Du an nay su dung ESP32, cam bien MPU6050 va module Quectel EC800K 4G-GPS de phat hien su co te nga, phat am thanh canh bao va gui tin nhan SMS chua toa do GPS toi mot so dien thoai da duoc cai dat san.

## 📁 Cau truc du an

```
mainproject/
├── main/                 # Vong lap chinh dieu khien he thong chua cac logic chinh va task cua chuong trinh
├── components/
│   ├── buzzer/           # Dieu khien buzzer va coi canh bao qua GPIO
│   ├── comm/             # Khoi tao UART va I2C
|   ├── led_indicator/    # Khoi dieu khien led
│   ├── debugs/           # In thong tin debug ra serial
│   ├── mpu6050/          # Doc du lieu tu cam bien
│   └── sim4g_gps/        # Giao tiep AT (gps, sms..) voi module EC800K
```

##  Tinh nang

- Phat hien te nga bang cam bien gia toc/goc
- Kich hoat buzzer canh bao
- kich hoat led nhay canh bao
- Lay toa do GPS qua module 4G
- Gui tin nhan SMS chua vi tri hien tai, neu khong lay duoc vi tri van gui tn
- Cau truc du an chuyen nghiep, chia module ro rang

##  Phan cung can thiet

- ESP32 DevKit C
- Cam bien MPU6050 (I2C)
- Module 4G-GPS Quectel EC800K (UART)
- Buzzer kich hoat GPIO
- Cap USB UART de debug

##  Cai dat va su dung

1. Cai [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Clone du an:

```bash
git clone https://github.com/wikibird2024/mainproject
cd mainproject
```

3. Build va flash:

```bash
idf.py fullclean
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

> Thay `/dev/ttyUSB0` bang cong serial cua ban

##  Giai thich thanh phan

- **mpu6050**: Doc du lieu raw tu cam bien MPU6050 bang I2C
- **sim4g_gps**: Giao tiep voi module EC800K de lay GPS va gui SMS
- **buzzer**: Dieu khien am thanh canh bao
- **led_indicator**: Dieu khien led
- **comm**: Khoi tao UART va I2C
- **debugs**: Ghi log thong tin ra serial

##  Nguyen ly phat hien te nga

1. Doc gia toc va toc do goc moi 200ms
2. Ap dung giai thuat nguong de phan tich te nga
3. Neu phat hien te nga:
   - Kich hoat buzzer
   - Lay toa do GPS
   - Gui tin nhan SMS

##  Vi du SMS

```
CANH BAO: Phat hien te nga!
Vi tri: Lat: 10.762622, Lon: 106.660172
```

##  Dinh huong mo rong

- Cai tien thuat toan bang machine learning
- Cho phep thay doi so dien thoai qua giao dien hoac NVS
- Them chuc nang gui du lieu qua HTTP

##  Giay phep
Original author: Tran Hao
Du an duoc cap phep theo MIT License (Massachusetts Institute of Technology – MIT).

