
# LED Indicator Component

This component controls a single LED connected to a GPIO pin, offering multiple modes such as:

- Always ON
- Always OFF
- Fast blinking
- Slow blinking
- Error blinking (double blink with pause)

## Features

- FreeRTOS task-based blink logic
- Configurable GPIO via `menuconfig`
- Simple API: init, set_mode, deinit
- Modular and reusable

## Configuration

Use `idf.py menuconfig` to change:

- LED GPIO pin
- Enable/disable LED logic

## Integration

Add to `CMakeLists.txt`:

```cmake
REQUIRES led_indicator
