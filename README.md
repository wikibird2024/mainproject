

---

# ESP32 Fall Detection System

A modular **ESP-IDF–based embedded system** designed to detect human falls using an inertial sensor, trigger local alerts, and report events remotely via cellular and MQTT communication.

The project follows a **component-oriented architecture**, emphasizing scalability, maintainability, and clear separation of concerns.

---

## Project Structure

```
mainproject/
├── CMakeLists.txt          # Top-level build configuration
├── sdkconfig               # Project configuration (generated)
├── sdkconfig.old           # Backup configuration
├── dependencies.lock       # Dependency lock file (ESP-IDF v5+)
├── readme.txt              # Project documentation
│
├── main/                   # Firmware entry point
│   ├── main.c              # System initialization and main tasks
│   ├── CMakeLists.txt
│   └── idf_component.yml   # External dependency declarations
│
├── components/             # Independent, reusable modules
│   ├── buzzer/             # Audible alert driver
│   ├── comm/               # UART communication abstraction (SIM interface)
│   ├── debugs/             # Logging and diagnostics utilities
│   ├── fall_logic/         # Core fall detection algorithm
│   ├── led_indicator/      # System status LED driver
│   ├── mpu6050/            # Motion sensor driver
│   ├── mqtt_client/        # MQTT JSON publisher
│   ├── sim4g_gps/          # 4G SIM EC800K (GPS + SMS)
│   ├── wifi_connect/       # Wi-Fi connection manager
│   └── bash.sh             # Helper script (recommended to move to /tools)
```

---

## System Overview

The system continuously monitors motion data from the **MPU6050** sensor to detect fall events.
Upon detecting a fall, it performs the following actions:

* Activates **local alerts** (buzzer and LED)
* Retrieves **GPS coordinates** via a 4G SIM module
* Sends an **SMS alert** with location data
* Publishes a **JSON event message** to an MQTT broker

The design prioritizes **reliability**, **clear data flow**, and **independent component development**.

---

## Core Components

* **wifi_connect**
  Manages Wi-Fi initialization and reconnection.

* **mqtt_client**
  Publishes structured JSON messages to a remote MQTT broker.

* **sim4g_gps**
  Interfaces with the EC800K 4G module for GPS positioning and SMS alerts.

* **mpu6050**
  Reads and processes motion data from the inertial sensor.

* **fall_logic**
  Implements the fall detection algorithm and decision-making logic.

* **buzzer / led_indicator**
  Provide immediate local feedback and system state indication.

* **comm**
  Generic UART communication layer used by external modules.

* **debugs**
  Centralized logging and diagnostic utilities.

---

## Execution Flow

1. System initialization
2. Wi-Fi connection establishment
3. MQTT broker connection
4. Continuous motion monitoring
5. Fall detection trigger:

   * Activate buzzer and LED
   * Acquire GPS location
   * Send SMS alert
   * Publish MQTT JSON message

---

## Design Principles

* Component isolation and reusability
* Hardware abstraction where applicable
* Clear separation between drivers, logic, and application flow
* ESP-IDF–native build and configuration system

---

## Environment

* **Framework:** ESP-IDF v5.x
* **Target:** ESP32
* **Language:** C

---

## Author

**wikibird**
July 2025

---
