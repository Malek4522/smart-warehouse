# Smart Warehouse ESP32 Firmware

This directory contains the firmware for ESP32 devices used in the Smart Warehouse system. The ESP32 functions as a mobile robot that performs tasks such as product putaway and retrieval.

## Features

- WebSocket communication with backend server
- Motor control for robot movement
- Sensor integration for navigation
- Task execution logic
- OTA (Over-The-Air) firmware updates
- Battery management

## Hardware Requirements

- ESP32 development board
- Motor drivers (L298N)
- DC motors or stepper motors
- Color sensors
- Distance sensors
- Battery and power management circuitry
- Chassis and mechanical components

## Files

- `esp32.ino` - Main firmware file for the ESP32 robots
- `stepperWithL298n.ino` - Helper code for controlling stepper motors using L298N motor driver
- `config.h.template` - Template configuration file (copy to `config.h` and modify)

## Installation

1. Install Arduino IDE
2. Install ESP32 board support: [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
3. Install required libraries:
   - ArduinoWebsockets
   - ArduinoJson
   - WiFi
   - Update

4. Connect your ESP32 to your computer
5. Create your configuration file:
   ```bash
   cp config.h.template config.h
   ```
6. Edit `config.h` with your network and hardware settings
7. Open the `esp32.ino` file in Arduino IDE
8. Upload the firmware to your ESP32

## Configuration

A `config.h.template` file is provided with all necessary configuration parameters. Important settings include:

```cpp
// WiFi credentials
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"

// Server connection
#define SERVER_ADDRESS "ws://your-server-ip:3000"
#define DEVICE_ID "ESP32_ROBOT_1"
```

Additional configurations include pin assignments for motors and sensors, operation parameters, and debug settings. Make a copy of this template to create your own `config.h` file.

## Motor Control

The system supports stepper motor control using the L298N motor driver. The `stepperWithL298n.ino` file contains specialized code for:

- Controlling stepper motor rotation
- Setting speed and direction
- Managing acceleration and deceleration
- Configuring motor pins and driver connections

## Functionality

The ESP32 device connects to the backend server via WebSocket and listens for tasks. When a task is received, it navigates to the specified location, performs the required operation (putaway or retrieval), and reports back to the server.

The firmware includes:
- Task queue management
- Navigation algorithms
- Product handling logic
- Self-diagnostics
- OTA update capability

## Development

To modify the firmware:
1. Make your changes to the code
2. Test thoroughly on a development bench
3. Upload to the production ESP32 devices using OTA updates

## Author

Made by MALEK - [abdelmalek.2004@outlook.com](mailto:abdelmalek.2004@outlook.com) 