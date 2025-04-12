#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Update.h>

// WiFi credentials
const char* ssid = "M";
const char* password = "malek2004";

// Server details - REPLACE WITH YOUR SERVER'S IP ADDRESS
const char* serverIp = "192.168.137.24";
const int serverPort = 3000;

// GPIO pin definitions
const int LED_PIN = 2; // Built-in LED on most ESP32 boards
// Variables for sensor data
int sensorValue = 0;
unsigned long lastDataSendTime = 0;
const unsigned long dataSendInterval = 5000; // 5 seconds

// ESP32 States
#define STATE_ACTIVE      "active"     // Normal operation
#define STATE_IDLE        "idle"       // No current task
#define STATE_CHARGING    "charging"   // At charging station
#define STATE_MAINTENANCE "maintenance" // Needs maintenance
#define STATE_SHUTDOWN    "shutdown"   // Device is shut down

// Task Types
#define TASK_PUTAWAY   "putaway"   // Placing item in storage
#define TASK_RETRIEVE  "retrieve"  // Getting item from storage

// Operation Phases
#define PHASE_GOING     "going"     // Going to target location
#define PHASE_RETURNING "returning" // Returning to home after operation

// Product Colors
#define PRODUCT_BLUE   "blue"
#define PRODUCT_RED    "red"
#define PRODUCT_GREEN  "green"
#define PRODUCT_YELLOW "yellow"

// Pre-defined warehouse locations with travel times (seconds) between them
struct LocationDistance {
  const char* from;
  const char* to;
  int timeSeconds;
};

// Map of location connections with travel times between them
const LocationDistance locationDistances[] = {
  {"HOME", "CHARGE", 7},
  {"HOME", "A1", 14},
  {"HOME", "B1", 28},
  {"HOME", "C1", 42},
  {"CHARGE", "A1", 7},
  {"CHARGE", "B1", 21},
  {"CHARGE", "C1", 35},
  {"A1", "A2", 14},
  {"A1", "B1", 14},
  {"A2", "A3", 14},
  {"A2", "B2", 14},
  {"A3", "B3", 14},
  {"B1", "B2", 14},
  {"B1", "C1", 14},
  {"B2", "B3", 14},
  {"B2", "C2", 14},
  {"B3", "C3", 14},
  {"C1", "C2", 14},
  {"C2", "C3", 14}
};

const int numLocationDistances = sizeof(locationDistances) / sizeof(LocationDistance);

// Additional variables for ESP32 status
float batteryLevel = 100.0;  // Battery percentage - will be calculated
bool isCharging = false;     // Charging status
String currentState = STATE_IDLE;  // Current state
bool isShutdown = false;     // Shutdown status

// Current task info
String currentLocation = "HOME";  // Current location name
String targetLocation = "HOME";   // Target location name
String taskId = "";               // Current task ID
String taskType = "";             // Current task type (putaway/retrieve)
String operationPhase = "";       // Current operation phase (going/returning)
bool isMoving = false;            // Whether the ESP32 is currently moving
unsigned long movementStartTime = 0; // When the current movement started
unsigned long totalMovementTime = 0; // How long the current movement will take

// Battery tracking variables
unsigned long lastBatteryUpdateTime = 0;
const unsigned long batteryUpdateInterval = 1000; // 1 second

// Variables for firmware update
struct FirmwareUpdate {
  String filename;
  size_t totalSize;
  int totalChunks;
  int receivedChunks;
  bool updateInProgress;
  String description;
};

FirmwareUpdate currentUpdate = {
  "", 0, 0, 0, false, ""
};

// Buffer for received update data
String updateBuffer = "";

// Socket.IO client
SocketIOclient socketIO;

// Additional variables for task info
String productColor = "";         // Current product color
String productId = "";            // Current product ID
String shelfNumber = "";          // Shelf number (e.g. S1, S2, S3, S4)
int levelNumber = 0;              // Shelf level number
int positionNumber = 0;           // Position number in the shelf

// **************************************************************************
// ROBOT HARDWARE INTERFACE
// **************************************************************************
// TO BE IMPLEMENTED BY ROBOTICS TEAM
// These functions provide the interface between the high-level control logic
// and the low-level hardware control. Replace the placeholders with actual
// hardware control code when implementing the physical robot.
// **************************************************************************

// Initialize robot sensors and motors
void initializeRobotHardware() {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Initialize motor drivers
  // - Set up sensor interfaces
  // - Perform calibration sequences
  // - Configure GPIO pins for hardware control
  // - Initialize any external components (grippers, arms, etc.)
  Serial.println("Robot hardware initialized");
}

// Start physical movement to a target location
void startRobotMovement(String from, String to) {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Calculate the path to take based on current and target locations
  // - Configure motor speeds/directions
  // - Activate drive system
  // - Start encoders/positioning system
  Serial.println("Robot movement started from " + from + " to " + to);
}

// Stop robot movement
void stopRobotMovement() {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Gradually decelerate motors (not abrupt stop)
  // - Turn off drive system once stopped
  // - Record final position
  // - Reset motion control variables
  Serial.println("Robot movement stopped");
}

// Perform shelf operation (picking or placing)
void performShelfOperation(String taskType, String shelfId, int level, int position) {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Position robot precisely at the shelf location
  // - Extend arm/gripper to the correct level and position
  // - Perform either grab (retrieve) or release (putaway) operation
  // - Return arm to safe position
  // - Verify operation success with sensors if possible
  Serial.println("Performing " + taskType + " operation at shelf " + shelfId + 
                 ", level " + String(level) + ", position " + String(position));
}

// Check if robot has physically arrived at destination
bool hasRobotArrived() {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Check positioning sensors
  // - Verify robot is within acceptable distance of target
  // - Confirm robot has stopped moving
  // - Return true only when physically at destination
  
  // For now, we'll just return true based on the time calculation in processMovement()
  return true;
}

// Report sensor status
void reportSensorStatus() {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Read all sensor values (distance, line following, etc.)
  // - Read battery voltage through analog pin
  // - Update global variables with sensor readings
  // - Detect any anomalies in sensor readings
  
  // For now, we just report a dummy value
  sensorValue = random(100, 1000);
}

// Check for obstacles in path
bool isPathClear() {
  // TODO: IMPLEMENT WITH HARDWARE CONTROL CODE
  // This function should:
  // - Check front-facing distance sensors
  // - Look for objects in path using IR or ultrasonic sensors
  // - Return false if minimum safe distance is violated
  // - Optionally check for line presence if using line following
  
  // For now, always return true
  return true;
}

// FUTURE HARDWARE FUNCTIONS TO CONSIDER:
// - calibrateSensors() - Recalibrate sensors during maintenance
// - emergencyStop() - Immediate stop for safety issues
// - batteryVoltageRead() - Read actual battery voltage
// - checkMotorTemperature() - Monitor for overheating
// - selfTest() - Run diagnostics on robot systems

// **************************************************************************
// END OF ROBOTICS IMPLEMENTATION SECTION
// **************************************************************************

void setup() {
  Serial.begin(115200);
  
  // Set pin modes
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize robot hardware
  initializeRobotHardware();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Connected to WiFi, IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup Socket.IO connection
  setupSocketIO();
  
  Serial.println("ESP32 is ready to communicate with server via Socket.IO");
}

void setupSocketIO() {
  // Server address, port and URL
  socketIO.begin(serverIp, serverPort);
  
  // Event handler
  socketIO.onEvent(socketIOEvent);
}

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  String connectMsg;
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.println("Socket.IO Disconnected");
      break;
    case sIOtype_CONNECT:
      Serial.println("Socket.IO Connected");
      // Send a message on connection
      connectMsg = "ESP32 connected!";
      socketIO.sendEVENT("[\"message\",\"" + connectMsg + "\"]");
      break;
    case sIOtype_EVENT:
      handleSocketIOEvent(payload, length);
      break;
    case sIOtype_ERROR:
      Serial.println("Socket.IO Error");
      break;
  }
}

void handleSocketIOEvent(uint8_t * payload, size_t length) {
  // Convert payload to string
  String message = String((char*)payload);
  Serial.println("Received event: " + message);
  
  // Parse the JSON event
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (!error) {
    // Extract event name and data
    String eventName = doc[0];
    Serial.print("Event: ");
    Serial.println(eventName);
    
    // Handle specific events
    if (eventName == "esp32_command") {
      JsonObject data = doc[1];
      executeCommand(data);
    } else if (eventName == "message") {
      String content = doc[1]["content"];
      Serial.print("Message: ");
      Serial.println(content);
    } else if (eventName == "assign_task") {
      // Handle new task assignment
      String taskId = doc[1]["taskId"];
      String taskType = doc[1]["taskType"];
      String productColorValue = doc[1]["productColor"]; // blue, red, green, yellow
      
      // Extract additional task details
      String productIdValue = doc[1]["productId"];
      String targetLocation = doc[1]["targetLocation"]; 
      
      // Extract shelf position details
      String shelfNumberValue = doc[1]["shelfNumber"];
      int levelNumberValue = doc[1].containsKey("levelNumber") ? doc[1]["levelNumber"] : 0;
      int positionNumberValue = doc[1].containsKey("positionNumber") ? doc[1]["positionNumber"] : 0;
      
      assignNewTask(taskId, taskType, productColorValue, productIdValue, targetLocation, 
                    shelfNumberValue, levelNumberValue, positionNumberValue);
    } else if (eventName == "update_data") {
      // Handle data update
      receiveDataUpdate(doc[1]);
    } else if (eventName == "shutdown") {
      // Handle shutdown command
      shutdownESP32();
    } else if (eventName == "power_on") {
      // Handle power on command
      powerOnESP32();
    } else if (eventName == "charge") {
      // Handle charging command
      goToCharging();
    } else if (eventName == "maintenance") {
      // Handle maintenance command
      goToMaintenance();
    } else if (eventName == "update_metadata") {
      // Handle update metadata
      receiveUpdateMetadata(doc[1]);
    } else if (eventName == "update_chunk") {
      // Handle update chunks
      receiveUpdateChunk(doc[1]);
    } else if (eventName == "update_aborted") {
      // Handle update abort
      abortUpdate();
    }
  } else {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
  }
}

void loop() {
  // Keep the Socket.IO connection alive
  socketIO.loop();
  
  // Calculate battery level
  updateBatteryLevel();
  
  // Process movement based on current state and task
  processMovement();
  
  // Update sensor readings
  reportSensorStatus();
  
  unsigned long currentMillis = millis();
  
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    // Send sensor data at regular intervals
    if (currentMillis - lastDataSendTime >= dataSendInterval) {
      lastDataSendTime = currentMillis;
      sendSensorData();
      
      // Also send status updates
      sendStatusUpdate();
    }
  } else {
    // Attempt to reconnect to WiFi
    Serial.println("WiFi disconnected, trying to reconnect...");
    WiFi.begin(ssid, password);
    delay(1000);
  }
  
  delay(100); // Short delay to prevent excessive looping
}

// Calculate battery level based on activity
void updateBatteryLevel() {
  unsigned long currentMillis = millis();
  
  // Update battery level every second
  if (currentMillis - lastBatteryUpdateTime >= batteryUpdateInterval) {
    lastBatteryUpdateTime = currentMillis;
    
    // TEMPORARY: Keep battery level at 100% until real battery reading is implemented
    batteryLevel = 100.0;
    
    // NOTE: The code below is commented out until real battery implementation
    /*
    // Don't update battery if shutdown
    if (isShutdown) {
      return;
    }
    
    // If charging, increase battery level
    if (isCharging) {
      batteryLevel += 0.05; // Charge at 5% per second
      if (batteryLevel > 100.0) {
        batteryLevel = 100.0;
      }
    } else {
      // Decrease battery based on state
      if (isMoving) {
        batteryLevel -= 0.02; // More power consumption when moving
      } else {
        batteryLevel -= 0.005; // Less power consumption when idle
      }
      
      // Ensure battery doesn't go below 0
      if (batteryLevel < 0) {
        batteryLevel = 0;
      }
    }
    
    // If battery is critically low, request charging
    if (batteryLevel < 10 && !isCharging && currentState != STATE_CHARGING && !isShutdown) {
      Serial.println("Battery critically low, requesting charging");
      goToCharging();
    }
    */
  }
}

// Process movement based on current state and target
void processMovement() {
  // Skip movement if shutdown or in maintenance
  if (isShutdown || currentState == STATE_MAINTENANCE) {
    return;
  }
  
  // If charging or going to charge
  if (currentState == STATE_CHARGING) {
    if (currentLocation != "CHARGE") {
      // Move towards charging station
      moveToLocation("CHARGE");
    } else {
      // At charging station, start charging
      isCharging = true;
    }
    return;
  }
  
  // If active and has a task
  if (currentState == STATE_ACTIVE && taskId != "") {
    if (currentLocation != targetLocation) {
      if (!isMoving) {
        // Start moving towards target location
        moveToLocation(targetLocation);
      } else {
        // Check if we've reached the destination
        if (isPathClear()) { // Check if path is clear
          unsigned long currentTime = millis();
          if (currentTime - movementStartTime >= totalMovementTime) {
            // Arrived at the destination
            isMoving = false;
            stopRobotMovement(); // Stop the physical robot
            currentLocation = targetLocation;
            Serial.println("Arrived at location: " + targetLocation);
            
            // Perform shelf operation if at a shelf
            if (targetLocation.startsWith("S") && operationPhase == PHASE_GOING) {
              performShelfOperation(taskType, shelfNumber, levelNumber, positionNumber);
            }
            
            // Send status update
            sendLocationArrivalUpdate();
            
            // If we've reached the task target, complete the task or return home
            if (currentLocation == targetLocation) {
              if (operationPhase == PHASE_GOING) {
                // If we were going to the shelf, now we need to return home
                moveToLocation("HOME");
              } else if (operationPhase == PHASE_RETURNING) {
                // If we've returned home, the task is now complete
                taskCompleted();
              }
            }
          }
        } else {
          // Path is not clear, might need to recalculate or wait
          Serial.println("Path to " + targetLocation + " is not clear");
        }
      }
    }
  }
}

// Start moving to a location
void moveToLocation(String target) {
  if (isMoving) {
    return; // Already moving
  }
  
  // Calculate travel time based on source and destination
  int travelTimeSeconds = getTravelTime(currentLocation, target);
  
  if (travelTimeSeconds < 99) { // If valid path exists
    isMoving = true;
    targetLocation = target;
    movementStartTime = millis();
    totalMovementTime = travelTimeSeconds * 1000; // Convert to milliseconds
    
    // Set operation phase if we're in an active task
    if (currentState == STATE_ACTIVE && taskId != "") {
      // If we're moving to a shelf, we're in "going" phase
      // If we're moving back to HOME, we're in "returning" phase
      if (target == "HOME" && currentLocation.startsWith("S")) {
        operationPhase = PHASE_RETURNING;
      } else if (target.startsWith("S")) {
        operationPhase = PHASE_GOING;
      }
      
      // Send phase update
      sendPhaseUpdate();
    }
    
    Serial.print("Starting movement from ");
    Serial.print(currentLocation);
    Serial.print(" to ");
    Serial.print(target);
    Serial.print(" (travel time: ");
    Serial.print(travelTimeSeconds);
    Serial.println(" seconds)");
    
    // Start the actual robot movement
    startRobotMovement(currentLocation, target);
    
    // Send movement start notification
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    // Add event name
    array.add("movement_started");
    
    // Add data object
    JsonObject data = array.createNestedObject();
    data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    data["from"] = currentLocation;
    data["to"] = target;
    data["travelTime"] = travelTimeSeconds;
    data["operationPhase"] = operationPhase;
    
    // Serialize JSON to string
    String output;
    serializeJson(doc, output);
    
    // Send via Socket.IO
    socketIO.sendEVENT(output);
  } else {
    Serial.println("No valid path found from " + currentLocation + " to " + target);
  }
}

// Send operation phase update
void sendPhaseUpdate() {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("operation_phase_update");
  
  // Add data object
  JsonObject data = array.createNestedObject();
  data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  data["taskId"] = taskId;
  data["taskType"] = taskType;
  data["operationPhase"] = operationPhase;
  data["currentLocation"] = currentLocation;
  data["targetLocation"] = targetLocation;
  data["productColor"] = productColor;
  data["shelfNumber"] = shelfNumber;
  data["levelNumber"] = levelNumber;
  data["positionNumber"] = positionNumber;
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  
  Serial.println("Phase update sent: " + operationPhase);
}

// Send location arrival update
void sendLocationArrivalUpdate() {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("location_arrival");
  
  // Add data object
  JsonObject data = array.createNestedObject();
  data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  data["location"] = currentLocation;
  data["taskId"] = taskId;
  data["taskType"] = taskType;
  data["operationPhase"] = operationPhase;
  data["productColor"] = productColor;
  data["shelfNumber"] = shelfNumber;
  data["levelNumber"] = levelNumber;
  data["positionNumber"] = positionNumber;
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  
  Serial.println("Location arrival update sent: " + currentLocation);
}

// Handle task completion notification
void taskCompleted() {
  Serial.println("Task completed: " + taskId);
  
  // Send task completion notification
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("task_completed");
  
  // Add data object
  JsonObject data = array.createNestedObject();
  data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  data["taskId"] = taskId;
  data["location"] = currentLocation;
  data["taskType"] = taskType;
  data["productColor"] = productColor;
  data["productId"] = productId;
  data["shelfNumber"] = shelfNumber;
  data["levelNumber"] = levelNumber;
  data["positionNumber"] = positionNumber;
  data["operationPhase"] = operationPhase;
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  
  // Reset task
  taskId = "";
  taskType = "";
  productColor = "";
  productId = "";
  shelfNumber = "";
  levelNumber = 0;
  positionNumber = 0;
  operationPhase = "";
  
  // Return to idle state
  currentState = STATE_IDLE;
}

void sendSensorData() {
  // Create JSON document
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("esp32_data");
  
  // Add data object
  JsonObject data = array.createNestedObject();
  data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  data["sensorValue"] = sensorValue;
  data["uptime"] = millis() / 1000;
  data["rssi"] = WiFi.RSSI();
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  Serial.println("Sensor data sent: " + output);
}

void executeCommand(JsonObject& command) {
  // Example: handle a 'toggle' command
  if (command["action"] == "toggle") {
    int pin = command["pin"];
    int value = command["value"];
    
    Serial.print("Toggling pin ");
    Serial.print(pin);
    Serial.print(" to value ");
    Serial.println(value);
    
    // Special case for the built-in LED
    if (pin == LED_PIN) {
      digitalWrite(LED_PIN, value);
      
      if (value == 1) {
        Serial.println("LED turned ON");
      } else {
        Serial.println("LED turned OFF");
      }
      
      // Send confirmation back to server
      DynamicJsonDocument doc(1024);
      JsonArray array = doc.to<JsonArray>();
      
      // Add event name
      array.add("confirmation");
      
      // Add data object
      JsonObject response = array.createNestedObject();
      response["action"] = "toggle";
      response["pin"] = pin;
      response["status"] = "success";
      response["value"] = value;
      
      String output;
      serializeJson(doc, output);
      socketIO.sendEVENT(output);
    } else {
      // For other pins
      digitalWrite(pin, value);
    }
  }
  
  // Add more command types as needed
}

// Send status update to server
void sendStatusUpdate() {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("status_update");
  
  // Add data object
  JsonObject data = array.createNestedObject();
  data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  data["batteryLevel"] = batteryLevel;
  data["currentLocation"] = currentLocation;
  data["targetLocation"] = targetLocation;
  data["state"] = currentState;
  data["isCharging"] = isCharging;
  data["isMoving"] = isMoving;
  data["taskId"] = taskId;
  data["taskType"] = taskType;
  data["productColor"] = productColor;
  data["productId"] = productId;
  data["shelfNumber"] = shelfNumber;
  data["levelNumber"] = levelNumber;
  data["positionNumber"] = positionNumber;
  data["operationPhase"] = operationPhase;
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  Serial.println("Status update sent: " + output);
}

// Handle new task assignment
void assignNewTask(String tId, String tType, String pColor, String pId, String target, 
                   String shelf, int level, int position) {
  Serial.println("New task assigned: " + tId);
  Serial.print("Task type: ");
  Serial.println(tType);
  Serial.print("Product color: ");
  Serial.println(pColor);
  Serial.print("Product ID: ");
  Serial.println(pId);
  Serial.print("Target location: ");
  Serial.println(target);
  Serial.print("Shelf number: ");
  Serial.println(shelf);
  Serial.print("Level number: ");
  Serial.println(level);
  Serial.print("Position number: ");
  Serial.println(position);
  
  // Update task variables
  taskId = tId;
  taskType = tType;
  productColor = pColor;
  productId = pId;
  targetLocation = target;
  shelfNumber = shelf;
  levelNumber = level;
  positionNumber = position;
  operationPhase = PHASE_GOING; // Initially going to the target
  
  // Change state to active
  currentState = STATE_ACTIVE;
  
  // Acknowledge task received
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("task_accepted");
  
  // Add data object
  JsonObject data = array.createNestedObject();
  data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  data["taskId"] = taskId;
  data["taskType"] = taskType;
  data["productColor"] = productColor;
  data["targetLocation"] = targetLocation;
  data["shelfNumber"] = shelfNumber;
  data["levelNumber"] = levelNumber;
  data["positionNumber"] = positionNumber;
  data["state"] = currentState;
  data["operationPhase"] = operationPhase;
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  
  // Begin moving to the target location
  moveToLocation(targetLocation);
}

// Handle data update from server
void receiveDataUpdate(JsonObject data) {
  // Process the update data
  Serial.println("Received data update from server");
  
  // Example: extract and use the data
  if (data.containsKey("parameter1")) {
    String parameter1 = data["parameter1"];
    Serial.println("Parameter 1: " + parameter1);
  }
  
  // Acknowledge update received
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("update_received");
  
  // Add data object
  JsonObject response = array.createNestedObject();
  response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  response["status"] = "success";
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
}

// Handle shutdown command
void shutdownESP32() {
  // If already shutdown, power on instead (toggle behavior)
  if (isShutdown) {
    powerOnESP32();
    return;
  }
  
  Serial.println("Shutdown command received");
  
  // Stop any current movement
  if (isMoving) {
    isMoving = false;
    stopRobotMovement();
  }
  
  // Cancel charging if active
  isCharging = false;
  
  // Set shutdown flag and update state
  isShutdown = true;
  currentState = STATE_SHUTDOWN;
  
  // Reset any active task
  taskId = "";
  taskType = "";
  operationPhase = "";
  
  // Acknowledge shutdown
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("shutdown_acknowledged");
  
  // Add data object
  JsonObject response = array.createNestedObject();
  response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  response["status"] = "shutting_down";
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
  
  // Send immediate status update to confirm shutdown state
  sendStatusUpdate();
}

// Handle power on command
void powerOnESP32() {
  // Only process if currently shutdown
  if (isShutdown) {
    Serial.println("Power on command received");
    
    // Clear shutdown flag and set to idle state
    isShutdown = false;
    currentState = STATE_IDLE;
    
    // Acknowledge power on
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    // Add event name
    array.add("power_on_acknowledged");
    
    // Add data object
    JsonObject response = array.createNestedObject();
    response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    response["status"] = "powered_on";
    
    // Serialize JSON to string
    String output;
    serializeJson(doc, output);
    
    // Send via Socket.IO
    socketIO.sendEVENT(output);
    
    // Send immediate status update to confirm the new state
    sendStatusUpdate();
  } else {
    Serial.println("Already powered on");
  }
}

// Handle charging command
void goToCharging() {
  // If already charging, go to idle instead (toggle behavior)
  if (currentState == STATE_CHARGING) {
    Serial.println("Exiting charging mode");
    currentState = STATE_IDLE;
    isCharging = false;
    
    // Acknowledge change to idle
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    // Add event name
    array.add("state_changed");
    
    // Add data object
    JsonObject response = array.createNestedObject();
    response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    response["previousState"] = STATE_CHARGING;
    response["currentState"] = STATE_IDLE;
    
    // Serialize JSON to string
    String output;
    serializeJson(doc, output);
    
    // Send via Socket.IO
    socketIO.sendEVENT(output);
    
    // Send status update
    sendStatusUpdate();
    return;
  }

  // Don't allow charging when shutdown
  if (isShutdown) {
    Serial.println("Cannot enter charging mode while shutdown");
    return;
  }
  
  Serial.println("Charging command received");
  
  // Set charging flag and update state
  currentState = STATE_CHARGING;
  targetLocation = "CHARGE";
  
  // Acknowledge charging
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("charging_acknowledged");
  
  // Add data object
  JsonObject response = array.createNestedObject();
  response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  response["status"] = "moving_to_charger";
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
}

// Handle maintenance command
void goToMaintenance() {
  // If already in maintenance, go to idle instead (toggle behavior)
  if (currentState == STATE_MAINTENANCE) {
    Serial.println("Exiting maintenance mode");
    currentState = STATE_IDLE;
    
    // Acknowledge change to idle
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    // Add event name
    array.add("state_changed");
    
    // Add data object
    JsonObject response = array.createNestedObject();
    response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    response["previousState"] = STATE_MAINTENANCE;
    response["currentState"] = STATE_IDLE;
    
    // Serialize JSON to string
    String output;
    serializeJson(doc, output);
    
    // Send via Socket.IO
    socketIO.sendEVENT(output);
    
    // Send status update
    sendStatusUpdate();
    return;
  }
  
  // Don't allow maintenance mode if shutdown
  if (isShutdown) {
    Serial.println("Cannot enter maintenance mode while shutdown");
    return;
  }
  
  Serial.println("Maintenance command received");
  
  // Update state to maintenance
  currentState = STATE_MAINTENANCE;
  
  // Acknowledge maintenance
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("maintenance_acknowledged");
  
  // Add data object
  JsonObject response = array.createNestedObject();
  response["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  response["status"] = "maintenance_mode";
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
}

// Get travel time between two locations
int getTravelTime(String from, String to) {
  // Check direct connections first
  for (int i = 0; i < numLocationDistances; i++) {
    if ((from == locationDistances[i].from && to == locationDistances[i].to) ||
        (from == locationDistances[i].to && to == locationDistances[i].from)) {
      return locationDistances[i].timeSeconds;
    }
  }
  
  // For now, return a fixed travel time (5 seconds) for any shelf locations that aren't directly connected
  // This mock implementation will allow the robot to "arrive" after 5 seconds
  if (to.startsWith("S") || from.startsWith("S")) {
    Serial.println("Using mock travel time (5s) for shelf route from " + from + " to " + to);
    return 5;
  }
  
  // If no direct connection, return a high value as default
  // In a real implementation, you would have a proper pathfinding algorithm
  return 99;
}

// Handle firmware update metadata
void receiveUpdateMetadata(JsonObject data) {
  // Extract metadata
  String filename = data["filename"];
  size_t size = data["size"];
  int totalChunks = data["totalChunks"];
  String description = data["description"];
  
  // Log update information
  Serial.println("====== FIRMWARE UPDATE STARTED ======");
  Serial.print("Filename: ");
  Serial.println(filename);
  Serial.print("Size: ");
  Serial.print(size);
  Serial.println(" bytes");
  Serial.print("Total chunks: ");
  Serial.println(totalChunks);
  Serial.print("Description: ");
  Serial.println(description);
  Serial.println("====================================");
  
  // Reset update counters
  currentUpdate.filename = filename;
  currentUpdate.totalSize = size;
  currentUpdate.totalChunks = totalChunks;
  currentUpdate.receivedChunks = 0;
  currentUpdate.updateInProgress = true;
  currentUpdate.description = description;
  
  // Clear any previous data
  updateBuffer = "";
  
  // Notify server that we've received the metadata
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  
  // Add event name
  array.add("update_metadata_received");
  
  // Add data object
  JsonObject metadata = array.createNestedObject();
  metadata["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  metadata["filename"] = filename;
  metadata["ready"] = true;
  
  // Serialize JSON to string
  String output;
  serializeJson(doc, output);
  
  // Send via Socket.IO
  socketIO.sendEVENT(output);
}

// Handle firmware update chunk
void receiveUpdateChunk(JsonObject data) {
  // Make sure an update is in progress
  if (!currentUpdate.updateInProgress) {
    Serial.println("Error: No update is in progress");
    return;
  }
  
  // Extract chunk data
  int chunkIndex = data["chunkIndex"];
  int totalChunks = data["totalChunks"];
  String chunkData = data["data"];
  bool isLastChunk = data["isLastChunk"];
  
  // Validate chunk
  if (chunkIndex != currentUpdate.receivedChunks) {
    Serial.print("Error: Expected chunk ");
    Serial.print(currentUpdate.receivedChunks);
    Serial.print(" but received chunk ");
    Serial.println(chunkIndex);
    return;
  }
  
  // Increment received chunks counter
  currentUpdate.receivedChunks++;
  
  // Progress message (only log at reasonable intervals to avoid spam)
  if (chunkIndex % 10 == 0 || isLastChunk) {
    Serial.print("Received chunk ");
    Serial.print(chunkIndex + 1);
    Serial.print(" of ");
    Serial.print(totalChunks);
    Serial.print(" (");
    Serial.print((chunkIndex + 1) * 100 / totalChunks);
    Serial.println("%)");
  }
  
  // If last chunk, finish the update
  if (isLastChunk) {
    Serial.println("All chunks received!");
    
    // Notify server that update was received
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    // Add event name
    array.add("update_applied");
    
    // Add data object
    JsonObject result = array.createNestedObject();
    result["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    result["filename"] = currentUpdate.filename;
    result["success"] = true;
    result["message"] = "Update received successfully (not applied - just for testing)";
    
    // Serialize JSON to string
    String output;
    serializeJson(doc, output);
    
    // Send via Socket.IO
    socketIO.sendEVENT(output);
    
    // Reset update state
    currentUpdate.updateInProgress = false;
    updateBuffer = "";
  }
}

// Abort the current update process
void abortUpdate() {
  if (currentUpdate.updateInProgress) {
    Serial.println("Update aborted by server");
    
    // Reset update state
    currentUpdate.updateInProgress = false;
    updateBuffer = "";
    
    // Notify server
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    
    // Add event name
    array.add("update_aborted_acknowledged");
    
    // Add data object
    JsonObject data = array.createNestedObject();
    data["deviceId"] = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    data["message"] = "Update aborted by request";
    
    // Serialize JSON to string
    String output;
    serializeJson(doc, output);
    
    // Send via Socket.IO
    socketIO.sendEVENT(output);
  }
}
