# Smart Warehouse Backend

This is the backend server for the Smart Warehouse system. It manages the warehouse operations, communicates with ESP32 devices, and provides an API for the frontend application.

## Features

- WebSocket communication with ESP32 robots
- RESTful API for frontend integration
- Database integration for warehouse data
- Task management system for product putaway and retrieval
- Firmware update management for ESP32 devices

## Technologies Used

- Node.js
- Express.js
- Socket.IO
- MongoDB
- Multer for file uploads

## Installation

1. Clone the repository
2. Navigate to the backend directory
3. Install dependencies:

```bash
npm install
```

4. Create a `.env` file (use `.env.template` as a reference):
```bash
cp .env.template .env
```

5. Edit the `.env` file with your specific configuration:
```
PORT=3000
MONGODB_URI=mongodb://localhost:27017/smartwarehouse
JWT_SECRET=your_jwt_secret
```

6. Start the server:

```bash
npm start
```

## Environment Variables

A `.env.template` file is provided with all required configuration options. The important variables include:

- `PORT` - The port on which the server runs
- `MONGODB_URI` - MongoDB connection string
- `JWT_SECRET` - Secret key for JWT token generation
- `ALLOWED_ORIGINS` - CORS allowed origins
- `WS_PORT` - WebSocket port (if different from HTTP port)
- `UPDATE_DIR` - Directory for ESP32 firmware updates

Make a copy of this template to create your own `.env` file.

## API Endpoints

- `/api/shelves` - Manage warehouse shelves
- `/api/tasks` - Manage robot tasks
- `/api/updates` - Manage ESP32 firmware updates

## WebSocket Events

The backend uses Socket.IO for real-time communication with ESP32 devices and frontend clients. Key events include:

- Task assignment
- Location updates
- Device status updates
- Firmware update management

## Directory Structure

- `/src/config` - Configuration files
- `/src/controllers` - Business logic
- `/src/middleware` - Express middleware
- `/src/models` - Database models
- `/src/routes` - API routes
- `/src/public` - Static files
- `/src/updates` - ESP32 firmware updates

## Author

Made by MALEK - [abdelmalek.2004@outlook.com](mailto:abdelmalek.2004@outlook.com) 