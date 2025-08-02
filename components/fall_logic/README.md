
# fall_logic - Fall Detection Component for ESP32

This component implements a modular fall detection logic using sensor data (typically from MPU6050), designed for ESP32 with ESP-IDF. It periodically samples sensor data and triggers fall detection based on configurable thresholds.

---

## 📌 Features

- Periodic checking of total acceleration vector.
- Configurable fall detection threshold and interval.
- Toggleable at runtime and via `menuconfig`.
- Lightweight FreeRTOS task-based design.
- Designed for integration with alerting modules (e.g., buzzer, 4G-SMS, MQTT).

---

## 🛠️ Configuration Options

Use `idf.py menuconfig` to access the following options:

| Option                                      | Default | Description |
|--------------------------------------------|---------|-------------|
| `CONFIG_FALL_LOGIC_ENABLE`                 | `y`     | Enable or disable the fall logic module. |
| `CONFIG_FALL_LOGIC_THRESHOLD`              | `0.5`   | Threshold (in g) to detect a fall. |
| `CONFIG_FALL_LOGIC_CHECK_INTERVAL_MS`      | `1000`  | Interval (ms) between checks. |
| `CONFIG_FALL_LOGIC_TASK_STACK_SIZE`        | `4096`  | Stack size for FreeRTOS fall detection task. |
| `CONFIG_FALL_LOGIC_TASK_PRIORITY`          | `5`     | Task priority. |

---

## 📦 Public API

```c
void fall_logic_init(void);
void fall_logic_enable(void);
void fall_logic_disable(void);
bool fall_logic_is_enabled(void);
