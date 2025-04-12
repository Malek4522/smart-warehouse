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
- Motor drivers
- DC motors or stepper motors
- Color sensors
- Distance sensors
- Battery and power management circuitry
- Chassis and mechanical components

## Installation

1. Install Arduino IDE
2. Install ESP32 board support: [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
3. Install required libraries:
   - ArduinoWebsockets
   - ArduinoJson
   - WiFi
   - Update

4. Connect your ESP32 to your computer
5. Open the `esp32.ino` file in Arduino IDE
6. Configure WiFi settings in the code
7. Upload the firmware to your ESP32

## Configuration

Update the following configuration in the `esp32.ino` file:

```cpp
// WiFi credentials
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";

// Server connection
const char* serverAddress = "ws://your-server-ip:3000";
```

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