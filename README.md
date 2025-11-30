# BLE E-Paper Sensor Display

A Bluetooth Low Energy (BLE) environmental sensor with e-paper display, built on Zephyr RTOS for the Seeed XIAO nRF52840 board.

## Features

- **BLE Environmental Sensing Service (ESS)** - Temperature and humidity sensor data over BLE
- **E-Paper Display** - 250x122 pixel SSD1680-based e-paper display
- **RGB LED Control** - PWM-controlled RGB LED via BLE service
- **Low Power** - Optimized for battery operation with e-paper display

## Hardware

- **Board**: Seeed XIAO nRF52840 (BLE Sense)
- **MCU**: Nordic nRF52840 (ARM Cortex-M4F)
- **Display**: WeAct 2.13" E-Paper (SSD1680 controller, 250x122 pixels)
- **Sensors**: Temperature & Humidity (via BLE ESS)

### Pin Connections

| Function | Pin | GPIO |
|----------|-----|------|
| SPI SCK  | D8  | P1.13 |
| SPI MOSI | D10 | P1.15 |
| SPI MISO | D9  | P1.14 |
| Display CS | D1 | P0.03 |
| Display DC | D3 | P0.29 |
| Display RST | D0 | P0.02 |
| Display BUSY | D2 | P0.28 |
| Red LED  | - | P0.26 |
| Green LED | - | P0.30 |
| Blue LED | - | P0.06 |

## Building

### Prerequisites

- [Zephyr RTOS SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- West build tool
- ARM GCC toolchain

### Build Commands

```bash
# Build the project
west build -b xiao_ble

# Flash to device
west flash

# Clean build
west build -t pristine
```

## BLE Services

### Environmental Sensing Service (0x181A)
- **Temperature**: Read temperature in Celsius
- **Humidity**: Read humidity percentage

### RGB LED Service (0xFFE0)
- Control RGB LED color via BLE write

## Display Functions

- `display_epaper_init()` - Initialize e-paper display
- `display_show_message(message)` - Display text message
- `display_update_sensors(temp, humidity)` - Display sensor readings
- `display_draw_white()` - Fill display with white
- `display_set_rotation(rotation)` - Set display orientation (0°, 90°, 180°, 270°)

## Project Structure

```
.
├── src/
│   └── main.c                  # Main application
├── include/
│   ├── display_epaper.h        # Display API header
│   ├── display_epaper_cfb.c    # Display implementation
│   ├── ble_rgb_service.h       # RGB LED BLE service
│   └── ble_ess_service.h       # Environmental Sensing Service
├── xiao_ble.overlay            # Device tree overlay
├── prj.conf                    # Zephyr configuration
└── CMakeLists.txt              # Build configuration
```

## Configuration

Key Zephyr configurations in `prj.conf`:
- Bluetooth LE support
- Display drivers (SSD16XX)
- Character Framebuffer (CFB)
- PWM for RGB LED
- Logging

## License

MIT License - see [LICENSE](LICENSE) file for details.

Copyright (c) 2025
