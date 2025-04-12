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

4. Create a `.env` file with the following variables:
```
PORT=3000
MONGODB_URI=mongodb://localhost:27017/smartwarehouse
JWT_SECRET=your_jwt_secret
```

5. Start the server:

```bash
npm start
```

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