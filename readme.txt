Here's the full English translation of your document:

---

# Fall Detection and GPS Location Warning System Using ESP32

This project uses the ESP32, an MPU6050 sensor, and the Quectel EC800K 4G-GPS module to detect falls, trigger an audible warning, and send an SMS containing GPS coordinates to a pre-configured phone number.

## 📁 Project Structure

```
mainproject/
├── main/                 # Main loop controlling system logic and tasks
├── components/
│   ├── buzzer/           # Controls buzzer and audible alerts via GPIO
│   ├── comm/             # Initializes UART and I2C communication
│   ├── led_indicator/    # Controls LED indicators
│   ├── debugs/           # Outputs debugging information to serial
│   ├── mpu6050/          # Reads data from the motion sensor
│   └── sim4g_gps/        # Handles AT commands (GPS, SMS, etc.) with the EC800K module
```

## Features

* Detects falls using accelerometer and gyroscope data
* Triggers a buzzer for audible alerts
* Activates LED blinking for visual alerts
* Retrieves GPS coordinates via 4G module
* Sends SMS containing current location; if location is unavailable, still sends a warning
* Professionally structured with clearly separated modules

## Required Hardware

* ESP32 DevKit C
* MPU6050 sensor (I2C interface)
* Quectel EC800K 4G-GPS module (UART interface)
* Buzzer (triggered via GPIO)
* USB-UART cable for debugging

## Setup and Usage

1. Install [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Clone the project:

```bash
git clone https://github.com/wikibird2024/mainproject
cd mainproject
```

3. Build and flash the firmware:

```bash
idf.py fullclean
idf.py set-target esp32
idf.py menuconfig
idf.py buil
idf.py -p /dev/ttyUSB0 flash monitor
```

> Replace `/dev/ttyUSB0` with your actual serial port

## Component Explanation

* **mpu6050**: Reads raw accelerometer and gyroscope data from the MPU6050 via I2C
* **sim4g\_gps**: Communicates with EC800K module to obtain GPS data and send SMS
* **buzzer**: Controls the audible alert system
* **led\_indicator**: Manages the LED indicators
* **comm**: Initializes and configures UART and I2C buses
* **debugs**: Logs system information to the serial monitor

## Fall Detection Algorithm

1. Reads accelerometer and gyroscope data every 200ms
2. Applies a threshold-based algorithm to detect a fall
3. Upon detection:

   * Triggers the buzzer
   * Retrieves GPS coordinates
   * Sends an SMS message

## SMS Example

```
WARNING: Fall detected!
Location: https://maps.google.com/?q=10.823456,106.629654
Time: 2023-12-15 14:30:45
```

## Future Enhancements

* Improve algorithm using machine learning techniques
* Allow phone number configuration via GUI or NVS
* Add HTTP data transmission capability

## License

Original author: Tran Hao
This project is licensed under the MIT License (Massachusetts Institute of Technology – MIT).
