# Smart Warehouse System

A complete warehouse automation system that uses ESP32-powered robots to manage inventory through automated putaway and retrieval operations.

## System Overview

The Smart Warehouse system consists of three main components:
- **Backend Server**: Manages the warehouse operations and communicates with ESP32 robots
- **ESP32 Firmware**: Powers the warehouse robots that perform physical tasks
- **Frontend Application**: User interface for warehouse management and monitoring

## Features

- **Automated Inventory Management**: Robot-assisted product putaway and retrieval
- **Real-time Monitoring**: Track robot location and warehouse status in real-time
- **Task Queue System**: Efficient management of warehouse operations
- **OTA Updates**: Remote firmware updates for ESP32 robots
- **Shelf Management**: Digital representation of physical warehouse space
- **WebSocket Communication**: Real-time bidirectional data exchange

## Architecture

```
┌─────────────┐     WebSocket     ┌─────────────┐      HTTP      ┌─────────────┐
│  ESP32 Robot ├──────────────────┤ Backend API ├────────────────┤  Frontend   │
└─────────────┘                   └─────────────┘                └─────────────┘
                                        │
                                        │ MongoDB
                                        │
                                  ┌─────▼─────┐
                                  │  Database  │
                                  └───────────┘
```

## Technology Stack

- **Backend**: Node.js, Express, Socket.IO, MongoDB
- **ESP32 Firmware**: Arduino, WebSockets, Motor Control Libraries
- **Frontend**: React, Material-UI, Socket.IO Client

## Getting Started

### Prerequisites
- Node.js and npm
- MongoDB
- Arduino IDE
- ESP32 development boards with required components

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/Malek4522/smart-warehouse.git
   cd smart-warehouse
   ```

2. **Backend Setup**
   - Navigate to the backend directory
   - Install dependencies with `npm install`
   - Create environment file: `cp .env.template .env`
   - Edit `.env` with your configuration
   - Start the server: `npm start`
   - See [Backend README](backend/README.md) for detailed instructions

3. **ESP32 Setup**
   - Navigate to the esp32 directory
   - Create configuration: `cp config.h.template config.h`
   - Edit `config.h` with your settings
   - Upload the firmware using Arduino IDE
   - See [ESP32 README](esp32/README.md) for detailed instructions

## Configuration Templates

To simplify setup, configuration templates are provided:

- **Backend**: `.env.template` - Environment variables for the Node.js server
- **ESP32**: `config.h.template` - Hardware and network configuration for ESP32 firmware

Copy these templates and modify them for your specific deployment environment.

## System Components

### Backend Server
The backend server handles warehouse management, robot communication, and provides APIs for the frontend. For more details, see the [Backend README](backend/README.md).

### ESP32 Firmware
The ESP32 firmware runs on the warehouse robots, enabling them to navigate and perform tasks. For more details, see the [ESP32 README](esp32/README.md).

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- ESP32 and Arduino communities
- Node.js and Socket.IO documentation
- MongoDB documentation

## Author

Made by MALEK - [abdelmalek.2004@outlook.com](mailto:abdelmalek.2004@outlook.com)