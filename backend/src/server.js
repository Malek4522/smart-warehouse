require('dotenv').config();

const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const { connectDB } = require('./config/database');
const routes = require('./routes/index');
const cookieParser = require('cookie-parser');
const { createServer } = require('http');
const { Server } = require('socket.io');
const Shelf = require('./models/shelfmodel');
const { initializeShelves } = require('./controllers/shelfcontroller');
const multer = require('multer');
const path = require('path');
const fs = require('fs');

// Connect to Database
connectDB();

// Initialize shelves if not already done
initializeShelves();

const app = express();
const httpServer = createServer(app);

// Setup file upload middleware
const updateStorage = multer.diskStorage({
  destination: function (req, file, cb) {
    const updateDir = path.join(__dirname, 'updates');
    if (!fs.existsSync(updateDir)) {
      fs.mkdirSync(updateDir, { recursive: true });
    }
    cb(null, updateDir);
  },
  filename: function (req, file, cb) {
    cb(null, 'esp32_update_' + Date.now() + path.extname(file.originalname));
  }
});

const upload = multer({ 
  storage: updateStorage,
  limits: { fileSize: 10 * 1024 * 1024 }, // 10MB max file size
  fileFilter: function (req, file, cb) {
    // Accept only certain extensions
    const filetypes = /bin|hex|ino|zip/;
    const extname = filetypes.test(path.extname(file.originalname).toLowerCase());
    if (extname) {
      return cb(null, true);
    }
    cb(new Error('Only update files (.bin, .hex, .ino, .zip) are allowed'));
  }
});

// Socket.IO setup with CORS
const io = new Server(httpServer, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"],
    transports: ['websocket', 'polling']
  },
  allowEIO3: true // Allow Socket.IO v3 clients
});

// Track available updates
let availableUpdates = [];

// Read existing updates at startup
(async function loadExistingUpdates() {
  const updateDir = path.join(__dirname, 'updates');
  if (fs.existsSync(updateDir)) {
    try {
      const files = fs.readdirSync(updateDir);
      availableUpdates = files.map(file => {
        const filePath = path.join(updateDir, file);
        const stats = fs.statSync(filePath);
        return {
          id: Date.now() + '-' + Math.random().toString(36).substr(2, 9),
          filename: file,
          path: filePath,
          size: stats.size,
          uploadDate: stats.mtime,
          description: 'Previously uploaded update'
        };
      });
      console.log(`Loaded ${availableUpdates.length} existing updates`);
    } catch (error) {
      console.error('Error loading existing updates:', error);
    }
  }
})();

// Track ESP32 status
let esp32Status = {
  currentState: 'idle',
  hasActiveTask: false,
  taskId: null,
  taskQueue: [],
  currentLocation: 'HOME'
};

// Function to process the next task in queue
function processNextQueuedTask() {
  // If ESP32 is busy or there are no queued tasks, don't do anything
  if (esp32Status.hasActiveTask || esp32Status.taskQueue.length === 0 || 
      esp32Status.currentState === 'maintenance' || 
      esp32Status.currentState === 'shutdown' || 
      esp32Status.currentState === 'charging') {
    return;
  }
  
  // Get the next task from the queue
  const nextTask = esp32Status.taskQueue.shift();
  
  // Update ESP32 status
  esp32Status.hasActiveTask = true;
  esp32Status.taskId = nextTask.taskId;
  
  // Forward task to ESP32
  io.emit('assign_task', nextTask);
  
  // Broadcast updated queue status
  broadcastQueueStatus();
  
  console.log(`Processing next queued task: ${nextTask.taskId}`);
}

// Function to broadcast current queue status to all clients
function broadcastQueueStatus() {
  const queueData = {
    queueSize: esp32Status.taskQueue.length,
    currentTaskId: esp32Status.taskId,
    queueItems: [...esp32Status.taskQueue]
  };
  
  // Add queue position to each task
  queueData.queueItems.forEach((task, index) => {
    task.queuePosition = index + 1;
  });
  
  // Add the current task if there is one
  if (esp32Status.hasActiveTask && esp32Status.taskId) {
    // Find the current active task and add it to the beginning of the queue items
    // The current task will have queuePosition null to indicate it's active
    queueData.queueItems = [
      {
        // Try to find more info about the active task
        ...(esp32Status.taskQueue.find(t => t.taskId === esp32Status.taskId) || {}),
        taskId: esp32Status.taskId,
        queuePosition: null // Active task has no queue position
      },
      ...queueData.queueItems
    ];
  }
  
  // Make sure queueSize reflects only the actual queue (not including active task)
  queueData.queueSize = esp32Status.taskQueue.length;
  
  io.emit('queue_status', queueData);
}

// WebSocket connection handling
io.on('connection', (socket) => {
  console.log('New client connected:', socket.id);

  // Send welcome message to the newly connected client
  socket.emit('message', { type: 'server', content: 'Welcome to Smart Warehouse WebSocket server!' });

  // Send available updates to clients when they connect
  socket.emit('available_updates', availableUpdates);

  // Handle generic message
  socket.on('message', (data) => {
    console.log('Received message:', data);
    // Echo the message back to acknowledge receipt
    socket.emit('message', { type: 'echo', content: `Echo: ${data}` });
  });

  // Handle ESP32 data
  socket.on('esp32_data', (data) => {
    console.log('Received data from ESP32:', data);
    // Broadcast the data to all connected clients except the sender
    socket.broadcast.emit('warehouse_update', data);
    // Acknowledge receipt of data
    socket.emit('message', { type: 'ack', content: 'Data received' });
  });

  // Handle web client commands
  socket.on('web_command', (command) => {
    console.log('Received command from web client:', command);
    // Broadcast the command to all clients (including ESP32)
    io.emit('esp32_command', command);
    // Confirm command was sent
    socket.emit('message', { type: 'ack', content: 'Command sent to ESP32' });
  });

  // Handle plain text message from ESP32 (e.g. "ESP32 connected!")
  socket.on('text', (text) => {
    console.log('Received text from client:', text);
    // Echo message to all clients
    io.emit('message', { type: 'client_message', content: text, from: socket.id });
  });

  // Handle ESP32 status updates
  socket.on('status_update', (data) => {
    console.log('Received status update from ESP32:', data);
    
    // Update the ESP32 status tracking
    if (data.state) {
      esp32Status.currentState = data.state;
    }
    if (data.currentLocation) {
      esp32Status.currentLocation = data.currentLocation;
    }
    if (data.taskId) {
      esp32Status.hasActiveTask = true;
      esp32Status.taskId = data.taskId;
    } else {
      esp32Status.hasActiveTask = false;
      esp32Status.taskId = null;
    }
    
    // Check if state has changed and log appropriately
    if (data.state) {
      switch (data.state) {
        case 'active':
          console.log('ESP32 is active and executing a task');
          break;
        case 'idle':
          console.log('ESP32 is idle');
          break;
        case 'charging':
          console.log('ESP32 is charging or moving to charging station');
          break;
        case 'maintenance':
          console.log('ESP32 is in maintenance mode');
          break;
      }
    }
    
    // Broadcast status to all web clients
    socket.broadcast.emit('esp32_status', data);
  });

  // Handle location arrival notifications
  socket.on('location_arrival', (data) => {
    console.log(`ESP32 arrived at location ${data.location}`, data);
    // Broadcast to all web clients
    socket.broadcast.emit('esp32_location_update', data);
  });

  // Handle task completion notification
  socket.on('task_completed', async (data) => {
    console.log('Task completed by ESP32:', data);
    console.log(`Completed ${data.taskType} task for ${data.productColor} product at ${data.location}`);
    
    // Update ESP32 status
    esp32Status.hasActiveTask = false;
    esp32Status.taskId = null;
    
    try {
      // Handle position update based on task type
      if (data.taskType === 'putaway' && data.shelfNumber && data.levelNumber && data.positionNumber) {
        // Find the shelf and update position status
        const shelf = await Shelf.findOne({ shelfNumber: data.shelfNumber });
        if (shelf) {
          const level = shelf.levels.find(l => l.levelNumber === parseInt(data.levelNumber));
          if (level) {
            const position = level.positions.find(p => p.positionNumber === parseInt(data.positionNumber));
            if (position) {
              position.isEmpty = false;
              position.status = 'filled';
              position.productId = data.productId || `P${Date.now()}`;
              position.productData = { color: data.productColor };
              position.lastUpdated = new Date();
              await shelf.save();
            }
          }
        }
      } else if (data.taskType === 'retrieve' && data.shelfNumber && data.levelNumber && data.positionNumber) {
        // Find the shelf and update position status
        const shelf = await Shelf.findOne({ shelfNumber: data.shelfNumber });
        if (shelf) {
          const level = shelf.levels.find(l => l.levelNumber === parseInt(data.levelNumber));
          if (level) {
            const position = level.positions.find(p => p.positionNumber === parseInt(data.positionNumber));
            if (position) {
              position.isEmpty = true;
              position.status = 'empty';
              position.productId = null;
              position.productData = null;
              position.lastUpdated = new Date();
              await shelf.save();
            }
          }
        }
      }
    } catch (error) {
      console.error('Error updating shelf position:', error);
    }
    
    // Broadcast task completion to all web clients
    socket.broadcast.emit('task_completed', data);
    
    // Process next task in queue after completion
    setTimeout(processNextQueuedTask, 1000);
    
    // Broadcast updated queue status
    broadcastQueueStatus();
  });

  // Handle get queue status request
  socket.on('get_queue_status', () => {
    broadcastQueueStatus();
  });

  // Handle task acceptance
  socket.on('task_accepted', (data) => {
    console.log('Task accepted by ESP32:', data);
    console.log(`ESP32 is heading to ${data.targetLocation}`);
    // Broadcast task acceptance to all web clients
    socket.broadcast.emit('task_accepted', data);
  });

  // Handle update confirmation
  socket.on('update_received', (data) => {
    console.log('Update confirmed by ESP32:', data);
    // Broadcast confirmation to all web clients
    socket.broadcast.emit('update_confirmed', data);
  });

  // Handle shutdown acknowledgment
  socket.on('shutdown_acknowledged', (data) => {
    console.log('Shutdown acknowledged by ESP32:', data);
    // Broadcast to all web clients
    socket.broadcast.emit('esp32_shutdown', data);
  });

  // Handle charging acknowledgment
  socket.on('charging_acknowledged', (data) => {
    console.log('Charging acknowledged by ESP32:', data);
    // Broadcast to all web clients
    socket.broadcast.emit('esp32_charging', data);
  });

  // Handle maintenance acknowledgment
  socket.on('maintenance_acknowledged', (data) => {
    console.log('Maintenance mode acknowledged by ESP32:', data);
    // Broadcast to all web clients
    socket.broadcast.emit('esp32_maintenance', data);
  });

  // Web client commands to ESP32
  
  // Assign new task to ESP32 - Product Putaway Task
  socket.on('assign_putaway_task', async (task) => {
    console.log('Assigning new putaway task to ESP32:', task);
    
    try {
      // Find available position for this color
      const shelves = await Shelf.find();
      let availablePosition = null;
      
      for (const shelf of shelves) {
        if (shelf.shelfColor === task.productColor) {
          for (const level of shelf.levels) {
            for (const position of level.positions) {
              if (position.isEmpty) {
                availablePosition = {
                  shelfNumber: shelf.shelfNumber,
                  levelNumber: level.levelNumber,
                  positionNumber: position.positionNumber,
                  color: position.color
                };
                break;
              }
            }
            if (availablePosition) break;
          }
          if (availablePosition) break;
        }
      }
      
      if (!availablePosition) {
        console.log(`No available positions for color ${task.productColor}`);
        socket.emit('message', { 
          type: 'error', 
          content: `No available positions for color ${task.productColor}` 
        });
        return;
      }
      
      // Reserve the position
      const shelf = await Shelf.findOne({ shelfNumber: availablePosition.shelfNumber });
      const level = shelf.levels.find(l => l.levelNumber === availablePosition.levelNumber);
      const position = level.positions.find(p => p.positionNumber === availablePosition.positionNumber);
      
      // Mark position as reserved
      position.isEmpty = false;
      position.status = 'reserved';
      position.productId = `P${Date.now()}`;
      position.lastUpdated = new Date();
      await shelf.save();
      
      // Create task for ESP32
      const taskData = {
        taskId: `TASK_${Date.now()}`,
        taskType: 'putaway',
        productColor: task.productColor,
        productId: position.productId,
        shelfNumber: availablePosition.shelfNumber,
        levelNumber: availablePosition.levelNumber,
        positionNumber: availablePosition.positionNumber,
        targetLocation: availablePosition.shelfNumber
      };
      
      // Add to queue or directly assign based on ESP32 status
      if (esp32Status.hasActiveTask) {
        // Add to queue
        esp32Status.taskQueue.push(taskData);
        
        // Notify client that task was queued
        socket.emit('task_queued', {
          taskId: taskData.taskId,
          queuePosition: esp32Status.taskQueue.length
        });
        
        console.log(`Task ${taskData.taskId} added to queue at position ${esp32Status.taskQueue.length}`);
      } else {
        // Directly assign task to ESP32
        esp32Status.hasActiveTask = true;
        esp32Status.taskId = taskData.taskId;
        
        // Forward task to ESP32
        io.emit('assign_task', taskData);
        
        // Confirm task was sent
        socket.emit('message', { 
          type: 'ack', 
          content: `Task assignment sent to ESP32: Put away ${task.productColor} product to ${availablePosition.shelfNumber} level ${availablePosition.levelNumber} position ${availablePosition.positionNumber}` 
        });
      }
      
      // Broadcast updated queue status
      broadcastQueueStatus();
    } catch (error) {
      console.error('Error creating putaway task:', error);
      socket.emit('message', { 
        type: 'error', 
        content: `Error creating task: ${error.message}` 
      });
    }
  });
  
  // Assign new task to ESP32 - Product Retrieval Task
  socket.on('assign_retrieval_task', async (task) => {
    console.log('Assigning new retrieval task to ESP32:', task);
    
    try {
      // Find a filled position with the requested color
      const shelves = await Shelf.find();
      let targetPosition = null;
      let shelf, level, position;
      
      for (const s of shelves) {
        if (s.shelfColor === task.productColor) {
          for (const l of s.levels) {
            for (const p of l.positions) {
              if (!p.isEmpty && p.status === 'filled') {
                targetPosition = {
                  shelfNumber: s.shelfNumber,
                  levelNumber: l.levelNumber,
                  positionNumber: p.positionNumber,
                  color: p.color,
                  productId: p.productId
                };
                shelf = s;
                level = l;
                position = p;
                break;
              }
            }
            if (targetPosition) break;
          }
          if (targetPosition) break;
        }
      }
      
      if (!targetPosition) {
        console.log(`No available ${task.productColor} products to retrieve`);
        socket.emit('message', { 
          type: 'error', 
          content: `No available ${task.productColor} products to retrieve` 
        });
        return;
      }
      
      // Mark the position as in_transit
      position.status = 'in_transit';
      position.lastUpdated = new Date();
      await shelf.save();
      
      // Create task for ESP32
      const taskData = {
        taskId: `TASK_${Date.now()}`,
        taskType: 'retrieve',
        productColor: targetPosition.color,
        productId: targetPosition.productId,
        shelfNumber: targetPosition.shelfNumber,
        levelNumber: targetPosition.levelNumber,
        positionNumber: targetPosition.positionNumber,
        targetLocation: targetPosition.shelfNumber
      };
      
      // Add to queue or directly assign based on ESP32 status
      if (esp32Status.hasActiveTask) {
        // Add to queue
        esp32Status.taskQueue.push(taskData);
        
        // Notify client that task was queued
        socket.emit('task_queued', {
          taskId: taskData.taskId,
          queuePosition: esp32Status.taskQueue.length
        });
        
        console.log(`Task ${taskData.taskId} added to queue at position ${esp32Status.taskQueue.length}`);
      } else {
        // Directly assign task to ESP32
        esp32Status.hasActiveTask = true;
        esp32Status.taskId = taskData.taskId;
        
        // Forward task to ESP32
        io.emit('assign_task', taskData);
        
        // Confirm task was sent
        socket.emit('message', { 
          type: 'ack', 
          content: `Task assignment sent to ESP32: Retrieve ${targetPosition.color} product from ${targetPosition.shelfNumber} level ${targetPosition.levelNumber} position ${targetPosition.positionNumber}` 
        });
      }
      
      // Broadcast updated queue status
      broadcastQueueStatus();
    } catch (error) {
      console.error('Error creating retrieval task:', error);
      socket.emit('message', { 
        type: 'error', 
        content: `Error creating task: ${error.message}` 
      });
    }
  });

  // Send update data to ESP32
  socket.on('update_data', (data) => {
    console.log('Sending update data to ESP32:', data);
    // Forward update to ESP32
    io.emit('update_data', data);
    // Confirm update was sent
    socket.emit('message', { type: 'ack', content: 'Update data sent to ESP32' });
  });

  // Send shutdown command to ESP32
  socket.on('shutdown', () => {
    console.log('Sending shutdown command to ESP32');
    // Send shutdown command to ESP32
    io.emit('shutdown', {});
    // Confirm command was sent
    socket.emit('message', { type: 'ack', content: 'Shutdown command sent to ESP32' });
  });

  // Send charging command to ESP32
  socket.on('charge', () => {
    console.log('Sending charging command to ESP32');
    // Send charging command to ESP32
    io.emit('charge', {});
    // Confirm command was sent
    socket.emit('message', { type: 'ack', content: 'Charging command sent to ESP32' });
  });

  // Send maintenance command to ESP32
  socket.on('maintenance', () => {
    console.log('Sending maintenance command to ESP32');
    // Send maintenance command to ESP32
    io.emit('maintenance', {});
    // Confirm command was sent
    socket.emit('message', { type: 'ack', content: 'Maintenance command sent to ESP32' });
  });

  // Handle client disconnection
  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
    // Notify other clients
    socket.broadcast.emit('message', { type: 'system', content: `Client ${socket.id} disconnected` });
  });

  // Handle errors
  socket.on('error', (error) => {
    console.error('Socket error:', error);
  });

  // Cancel a queued task
  socket.on('cancel_task', (data) => {
    const { taskId } = data;
    
    // Check if it's the current task
    if (esp32Status.taskId === taskId) {
      socket.emit('message', {
        type: 'error',
        content: 'Cannot cancel active task. Wait for completion or send a stop command.'
      });
      return;
    }
    
    // Find and remove from queue
    const taskIndex = esp32Status.taskQueue.findIndex(t => t.taskId === taskId);
    
    if (taskIndex !== -1) {
      // Get task data
      const task = esp32Status.taskQueue[taskIndex];
      
      // Remove from queue
      esp32Status.taskQueue.splice(taskIndex, 1);
      
      console.log(`Task ${taskId} removed from queue`);
      
      // If this was a putaway task, we need to release the reserved position
      if (task.taskType === 'putaway' && task.shelfNumber && task.levelNumber && task.positionNumber) {
        // Release the position asynchronously
        (async () => {
          try {
            const shelf = await Shelf.findOne({ shelfNumber: task.shelfNumber });
            if (shelf) {
              const level = shelf.levels.find(l => l.levelNumber === parseInt(task.levelNumber));
              if (level) {
                const position = level.positions.find(p => p.positionNumber === parseInt(task.positionNumber));
                if (position && position.status === 'reserved') {
                  position.isEmpty = true;
                  position.status = 'empty';
                  position.productId = null;
                  position.lastUpdated = new Date();
                  await shelf.save();
                  console.log(`Released reserved position for canceled task ${taskId}`);
                }
              }
            }
          } catch (error) {
            console.error('Error releasing reserved position:', error);
          }
        })();
      }
      
      // Notify client
      socket.emit('message', {
        type: 'success',
        content: `Task ${taskId} canceled successfully`
      });
      
      // Broadcast updated queue status
      broadcastQueueStatus();
    } else {
      socket.emit('message', {
        type: 'error',
        content: `Task ${taskId} not found in queue`
      });
    }
  });

  // Start a specific task from the queue
  socket.on('start_task', (data) => {
    const { taskId } = data;
    
    // If ESP32 is busy, reject request
    if (esp32Status.hasActiveTask) {
      socket.emit('message', {
        type: 'error',
        content: 'ESP32 is already executing a task'
      });
      return;
    }
    
    // Find task in queue
    const taskIndex = esp32Status.taskQueue.findIndex(t => t.taskId === taskId);
    
    if (taskIndex !== -1) {
      // Get and remove task from queue
      const task = esp32Status.taskQueue.splice(taskIndex, 1)[0];
      
      // Update ESP32 status
      esp32Status.hasActiveTask = true;
      esp32Status.taskId = task.taskId;
      
      // Forward task to ESP32
      io.emit('assign_task', task);
      
      console.log(`Starting task ${taskId} from queue`);
      
      // Notify client
      socket.emit('message', {
        type: 'success',
        content: `Task ${taskId} started`
      });
      
      // Broadcast updated queue status
      broadcastQueueStatus();
    } else {
      socket.emit('message', {
        type: 'error',
        content: `Task ${taskId} not found in queue`
      });
    }
  });

  // Handle socket disconnect
  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
    
    // No action needed for client disconnects
  });

  // Need to handle ESP32 disconnect specially
  socket.on('esp32_disconnect', async (data) => {
    console.log('ESP32 disconnected:', data);
    
    // Mark ESP32 as unavailable
    esp32Status.currentState = 'offline';
    
    // If ESP32 had an active task, mark it as failed and release any reserved positions
    if (esp32Status.hasActiveTask && esp32Status.taskId) {
      console.log(`ESP32 disconnected while executing task ${esp32Status.taskId}`);
      
      // Find the task in the queue
      const activeTask = esp32Status.taskQueue.find(t => t.taskId === esp32Status.taskId);
      
      if (activeTask && activeTask.taskType === 'putaway') {
        // Release the reserved position
        try {
          const shelf = await Shelf.findOne({ shelfNumber: activeTask.shelfNumber });
          if (shelf) {
            const level = shelf.levels.find(l => l.levelNumber === parseInt(activeTask.levelNumber));
            if (level) {
              const position = level.positions.find(p => p.positionNumber === parseInt(activeTask.positionNumber));
              if (position && position.status === 'reserved') {
                position.isEmpty = true;
                position.status = 'empty';
                position.productId = null;
                position.lastUpdated = new Date();
                await shelf.save();
                console.log(`Released reserved position for failed task ${activeTask.taskId}`);
              }
            }
          }
        } catch (error) {
          console.error('Error releasing reserved position for failed task:', error);
        }
      }
      
      // Clear the active task
      esp32Status.hasActiveTask = false;
      esp32Status.taskId = null;
      
      // Broadcast to all clients
      socket.broadcast.emit('esp32_offline', { message: 'ESP32 is offline' });
    }
  });
  
  // Endpoint to clean up any orphaned reserved positions
  socket.on('clean_reserved_positions', async () => {
    try {
      console.log('Cleaning up orphaned reserved positions');
      
      // Find all reserved positions
      const shelves = await Shelf.find();
      let cleanedCount = 0;
      
      for (const shelf of shelves) {
        let shelfUpdated = false;
        
        for (const level of shelf.levels) {
          for (const position of level.positions) {
            if (position.status === 'reserved') {
              // Check if this position is for an active or queued task
              const isInUse = esp32Status.taskId === position.productId || 
                              esp32Status.taskQueue.some(t => t.productId === position.productId);
              
              if (!isInUse) {
                // No active task is using this position, release it
                position.isEmpty = true;
                position.status = 'empty';
                position.productId = null;
                position.productData = null;
                position.lastUpdated = new Date();
                shelfUpdated = true;
                cleanedCount++;
              }
            }
          }
        }
        
        if (shelfUpdated) {
          await shelf.save();
        }
      }
      
      socket.emit('message', {
        type: 'success',
        content: `Cleaned up ${cleanedCount} orphaned reserved positions`
      });
      
      // Update all clients
      io.emit('shelves_updated');
    } catch (error) {
      console.error('Error cleaning reserved positions:', error);
      socket.emit('message', {
        type: 'error',
        content: `Error cleaning reserved positions: ${error.message}`
      });
    }
  });

  // Handle update request from web client
  socket.on('request_available_updates', () => {
    socket.emit('available_updates', availableUpdates);
  });

  // Handle send update to ESP32
  socket.on('send_update_to_esp32', async (data) => {
    const { updateId } = data;
    
    // Find the update by ID
    const update = availableUpdates.find(u => u.id === updateId);
    
    if (!update) {
      socket.emit('message', { 
        type: 'error', 
        content: `Update with ID ${updateId} not found` 
      });
      return;
    }
    
    try {
      // Read the update file
      const updateData = fs.readFileSync(update.path);
      
      // Convert to base64 for transmission
      const base64Data = updateData.toString('base64');
      
      // Calculate chunks (50KB each)
      const chunkSize = 50 * 1024;
      const totalChunks = Math.ceil(base64Data.length / chunkSize);
      
      console.log(`Sending update ${update.filename} to ESP32 (${totalChunks} chunks)`);
      
      // Send update metadata to ESP32
      io.emit('update_metadata', {
        filename: update.filename,
        size: update.size,
        totalChunks: totalChunks,
        description: update.description || 'ESP32 Firmware Update'
      });
      
      // Send confirmation to client
      socket.emit('message', { 
        type: 'info', 
        content: `Sending update ${update.filename} to ESP32 (${totalChunks} chunks)` 
      });
      
      // Set minimal delay between chunks to avoid overwhelming ESP32
      const chunkDelay = 200; // 200ms delay between chunks
      
      // Start sending chunks
      for (let i = 0; i < totalChunks; i++) {
        const start = i * chunkSize;
        const end = Math.min(start + chunkSize, base64Data.length);
        const chunk = base64Data.substring(start, end);
        
        // Wait for the delay before sending next chunk
        await new Promise(resolve => setTimeout(resolve, chunkDelay));
        
        // Send chunk to ESP32
        io.emit('update_chunk', {
          chunkIndex: i,
          totalChunks: totalChunks,
          data: chunk,
          isLastChunk: i === totalChunks - 1
        });
        
        // Log progress every 10 chunks
        if (i % 10 === 0 || i === totalChunks - 1) {
          console.log(`Sent chunk ${i+1}/${totalChunks}`);
        }
      }
      
      console.log(`Completed sending update ${update.filename} to ESP32`);
      
      // Send update complete notification
      io.emit('update_complete', {
        filename: update.filename,
        timestamp: new Date().toISOString()
      });
      
      // Notify the web client
      socket.emit('message', { 
        type: 'success', 
        content: `Update ${update.filename} sent to ESP32 successfully` 
      });
    } catch (error) {
      console.error('Error sending update to ESP32:', error);
      socket.emit('message', { 
        type: 'error', 
        content: `Error sending update: ${error.message}` 
      });
    }
  });
  
  // Handle abort update
  socket.on('abort_update', () => {
    console.log('Update aborted by web client');
    io.emit('update_aborted', {
      timestamp: new Date().toISOString()
    });
    socket.emit('message', { 
      type: 'info', 
      content: 'Update aborted' 
    });
  });
});

// Middleware
app.use(cors());
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(express.static('src/public'));
app.use(cookieParser());

// Routes
app.use('/api', routes);

// Basic route
app.get('/', (req, res) => {
  res.json({ message: 'Welcome to Smart Warehouse API' });
});

// File upload route
app.post('/api/upload-update', upload.single('updateFile'), (req, res) => {
  try {
    // Check if file was uploaded
    if (!req.file) {
      return res.status(400).json({
        success: false,
        message: 'No file uploaded',
      });
    }
    
    // Save update info
    const newUpdate = {
      id: Date.now() + '-' + Math.random().toString(36).substr(2, 9),
      filename: req.file.filename,
      originalName: req.file.originalname,
      path: req.file.path,
      size: req.file.size,
      uploadDate: new Date(),
      description: req.body.description || 'ESP32 Firmware Update'
    };
    
    // Add to available updates
    availableUpdates.push(newUpdate);
    
    // Broadcast to all connected clients
    io.emit('new_update_available', newUpdate);
    
    console.log('New update file uploaded:', newUpdate.filename);
    
    // Return success with the update object
    res.status(200).json({
      success: true,
      message: 'Update file uploaded successfully',
      update: newUpdate
    });
  } catch (error) {
    console.error('Error uploading update file:', error);
    res.status(500).json({
      success: false,
      message: 'Error uploading update file',
      error: error.message
    });
  }
});

// Get list of available updates
app.get('/api/available-updates', (req, res) => {
  res.status(200).json(availableUpdates);
});

// Delete an update
app.delete('/api/updates/:id', (req, res) => {
  const updateId = req.params.id;
  const updateIndex = availableUpdates.findIndex(u => u.id === updateId);
  
  if (updateIndex === -1) {
    return res.status(404).json({
      message: 'Update not found'
    });
  }
  
  try {
    const update = availableUpdates[updateIndex];
    
    // Delete the file
    if (fs.existsSync(update.path)) {
      fs.unlinkSync(update.path);
    }
    
    // Remove from available updates
    availableUpdates.splice(updateIndex, 1);
    
    // Broadcast to all connected clients
    io.emit('update_deleted', { id: updateId });
    
    res.status(200).json({
      message: 'Update deleted successfully'
    });
  } catch (error) {
    console.error('Error deleting update:', error);
    res.status(500).json({
      message: 'Error deleting update',
      error: error.message
    });
  }
});

// Error handling middleware
app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).json({ 
        message: 'Something went wrong!',
        error: process.env.NODE_ENV === 'development' ? err.message : undefined
    });
});

// Set port and start server
const PORT = process.env.PORT || 3000;
httpServer.listen(PORT, '0.0.0.0', () => {
    console.log(`Server is running on port ${PORT}`);
    console.log(`WebSocket server is ready for connections`);
});

// Handle unhandled promise rejections
process.on('unhandledRejection', (err) => {
    console.log(`Error: ${err.message}`);
    // Close server & exit process
    httpServer.close(() => process.exit(1));
});