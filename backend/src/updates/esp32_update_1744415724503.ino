/**
 * ESP32 RC Car Controller
 * Version: 3.0
 * Author: mohamad lounnas
 * Last Updated: 2024
 *
 * Features:
 * - Dual Network Mode (AP + Home WiFi)
 * - Web-based Control Interface
 * - Real-time WebSocket Communication
 * - Tank-style Motor Control
 * - LED Status Indicators
 * - Comprehensive Debugging
 *
 * Hardware Requirements:
 * - ESP32 Development Board
 * - L298N Motor Driver (HW-095)
 * - 2x DC Motors
 * - 2x Status LEDs
 * - Power Supply (7.4V-12V)
 *
 * Pin Configuration:
 * Left Motor:
 *   - ENA: GPIO25 (PWM)
 *   - IN1: GPIO26
 *   - IN2: GPIO27
 * Right Motor:
 *   - ENB: GPIO13 (PWM)
 *   - IN3: GPIO14
 *   - IN4: GPIO12
 * Status LEDs:
 *   - Ready: GPIO4
 *   - Connection: GPIO17
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "esp32-hal-timer.h"
#include "esp32-hal-cpu.h"
#include <EEPROM.h>

bool DEBUG = true;
bool PRODUCTION_MODE = true;

//=============== CONFIGURATION ===============

// Network Configuration
struct NetworkConfig
{
    // Access Point Settings
    const char *ap_ssid = "RC_Car_Network";
    const char *ap_pass = "rc_car_123";
    IPAddress ap_ip{192, 168, 4, 1};
    IPAddress ap_gateway{192, 168, 4, 1};
    IPAddress ap_subnet{255, 255, 255, 0};

    // Home WiFi Settings
    const char *home_ssid = "Fares";
    const char *home_pass = "fareshaoua8888";

    // Communication Ports
    const uint16_t udp_port = 4210;
    const uint16_t websocket_port = 81;
    const uint16_t web_port = 80;

    // Communication Settings
    const uint16_t udp_packet_size = 32;
    const uint16_t json_buffer_size = 200;
} config;

// Pin Configuration
struct PinConfig
{
    // Motor Driver Pins
    const uint8_t ena = 13; // Left motor speed
    const uint8_t in1 = 14; // Left motor direction 1
    const uint8_t in2 = 12; // Left motor direction 2
    const uint8_t enb = 25; // Right motor speed
    const uint8_t in3 = 26; // Right motor direction 1
    const uint8_t in4 = 27; // Right motor direction 2

    // Status LED Pins
    const uint8_t led_ready = 4;       // Ready indicator
    const uint8_t led_connection = 17; // Connection indicator
} pins;

// Timing Configuration
struct TimingConfig
{
    const uint32_t command_timeout = 500;     // Motor stop timeout (ms)
    const uint32_t debug_interval = 1000;     // Debug print interval (ms)
    const uint32_t web_update_interval = 20;  // Increased WebSocket rate to 50Hz (was 50ms)
    const uint32_t ready_delay = 3000;        // Startup delay (ms)
    const uint32_t connection_blink = 100;    // LED blink interval (ms)
    const uint32_t connection_timeout = 1000; // Connection LED timeout (ms)
} timing;

// Add EEPROM Configuration before MotorConfig
struct EEPROMConfig
{
    static const int EEPROM_SIZE = 512;              // Size of EEPROM to initialize
    static const int CONFIG_START_ADDR = 0;          // Starting address for config data
    static const uint32_t MAGIC_NUMBER = 0xABCD1234; // For validation

    struct SavedConfig
    {
        uint32_t magic; // To verify if EEPROM was written by our program
        int deadzone;
        int min_power;
        float exp_factor;
        float log_factor;
        float boost_factor;
        uint8_t mode; // Changed from MotorConfig::ControlMode to uint8_t to avoid circular dependency
    };
};

// Motor Control Configuration
struct MotorConfig
{
    // Control modes
    enum ControlMode
    {
        LINEAR = 0,
        EXPONENTIAL = 1,
        LOGARITHMIC = 2
    };

    // Default values - using constexpr for compile-time constants
    static constexpr int DEFAULT_DEADZONE = 8;
    static constexpr int DEFAULT_MIN_POWER = 45;
    static constexpr float DEFAULT_EXP_FACTOR = 2.0f;
    static constexpr float DEFAULT_LOG_FACTOR = 2.0f;
    static constexpr float DEFAULT_SCALE_FACTOR = 2.55f;
    static constexpr int DEFAULT_MAX_SPEED = 255;
    static constexpr int DEFAULT_MIN_SPEED = -255;
    static constexpr float DEFAULT_BOOST_FACTOR = 1.2f;
    static constexpr ControlMode DEFAULT_MODE = EXPONENTIAL;

    // Runtime configurable values
    int deadzone;       // Joystick deadzone (0-100)
    int min_power;      // Minimum power needed to move
    float exp_factor;   // Exponential curve factor
    float log_factor;   // Logarithmic curve factor
    float boost_factor; // High-speed boost multiplier
    ControlMode mode;   // Current control mode

    // Fixed values
    const float scale_factor;
    const int max_speed;
    const int min_speed;

    // Constructor
    MotorConfig() : deadzone(DEFAULT_DEADZONE),
                    min_power(DEFAULT_MIN_POWER),
                    exp_factor(DEFAULT_EXP_FACTOR),
                    log_factor(DEFAULT_LOG_FACTOR),
                    boost_factor(DEFAULT_BOOST_FACTOR),
                    mode(DEFAULT_MODE),
                    scale_factor(DEFAULT_SCALE_FACTOR),
                    max_speed(DEFAULT_MAX_SPEED),
                    min_speed(DEFAULT_MIN_SPEED)
    {
    }

    // Method to validate and update configuration
    void updateConfig(int new_deadzone, int new_min_power, float new_exp_factor, float new_log_factor, float new_boost_factor, int new_mode)
    {
        deadzone = constrain(new_deadzone, 0, 100);              // 0-100%
        min_power = constrain(new_min_power, 0, 255);            // 0-255
        exp_factor = constrain(new_exp_factor, 0.1f, 50.0f);     // Increased to 50.0
        log_factor = constrain(new_log_factor, 0.1f, 10.0f);     // 0.1-10.0
        boost_factor = constrain(new_boost_factor, 1.0f, 30.0f); // 1.0-30.0
        mode = static_cast<ControlMode>(constrain(new_mode, 0, 2));

        // Save to EEPROM after updating
        saveToEEPROM();

        if (DEBUG)
        {
            Serial.println("\n=== Motor Configuration Updated ===");
            Serial.printf("Deadzone: %d\n", deadzone);
            Serial.printf("Min Power: %d\n", min_power);
            Serial.printf("Exp Factor: %.2f\n", exp_factor);
            Serial.printf("Log Factor: %.2f\n", log_factor);
            Serial.printf("Boost Factor: %.2f\n", boost_factor);
            Serial.printf("Mode: %d\n", mode);
            Serial.println("================================\n");
        }
    }

    void resetToDefaults()
    {
        deadzone = DEFAULT_DEADZONE;
        min_power = DEFAULT_MIN_POWER;
        exp_factor = DEFAULT_EXP_FACTOR;
        log_factor = DEFAULT_LOG_FACTOR;
        boost_factor = DEFAULT_BOOST_FACTOR;
        mode = DEFAULT_MODE;

        // Save defaults to EEPROM
        saveToEEPROM();
    }

    void getConfigJson(JsonDocument &doc)
    {
        doc["deadzone"] = deadzone;
        doc["min_power"] = min_power;
        doc["exp_factor"] = exp_factor;
        doc["log_factor"] = log_factor;
        doc["boost_factor"] = boost_factor;
        doc["mode"] = static_cast<int>(mode);
        doc["max_speed"] = max_speed;
        doc["min_speed"] = min_speed;
    }

    // Add these methods to the MotorConfig struct inside the existing MotorConfig definition
    void saveToEEPROM()
    {
        EEPROMConfig::SavedConfig config;
        config.magic = EEPROMConfig::MAGIC_NUMBER;
        config.deadzone = deadzone;
        config.min_power = min_power;
        config.exp_factor = exp_factor;
        config.log_factor = log_factor;
        config.boost_factor = boost_factor;
        config.mode = static_cast<uint8_t>(mode);

        EEPROM.put(EEPROMConfig::CONFIG_START_ADDR, config);
        EEPROM.commit();

        if (DEBUG)
        {
            Serial.println("\n=== Configuration Saved to EEPROM ===");
            Serial.printf("Deadzone: %d\n", deadzone);
            Serial.printf("Min Power: %d\n", min_power);
            Serial.printf("Exp Factor: %.2f\n", exp_factor);
            Serial.printf("Log Factor: %.2f\n", log_factor);
            Serial.printf("Boost Factor: %.2f\n", boost_factor);
            Serial.printf("Mode: %d\n", mode);
            Serial.println("===================================\n");
        }
    }

    bool loadFromEEPROM()
    {
        EEPROMConfig::SavedConfig config;
        EEPROM.get(EEPROMConfig::CONFIG_START_ADDR, config);

        // Check if the EEPROM contains valid data
        if (config.magic != EEPROMConfig::MAGIC_NUMBER)
        {
            if (DEBUG)
            {
                Serial.println("[EEPROM] No valid configuration found, using defaults");
            }
            return false;
        }

        // Load values
        deadzone = config.deadzone;
        min_power = config.min_power;
        exp_factor = config.exp_factor;
        log_factor = config.log_factor;
        boost_factor = config.boost_factor;
        mode = static_cast<ControlMode>(config.mode);

        if (DEBUG)
        {
            Serial.println("\n=== Configuration Loaded from EEPROM ===");
            Serial.printf("Deadzone: %d\n", deadzone);
            Serial.printf("Min Power: %d\n", min_power);
            Serial.printf("Exp Factor: %.2f\n", exp_factor);
            Serial.printf("Log Factor: %.2f\n", log_factor);
            Serial.printf("Boost Factor: %.2f\n", boost_factor);
            Serial.printf("Mode: %d\n", mode);
            Serial.println("======================================\n");
        }

        return true;
    }
} motor_config;

//=============== GLOBAL VARIABLES ===============

// Network Components
WiFiUDP udp;
WebServer server(config.web_port);
WebSocketsServer webSocket(config.websocket_port);

// System Status
struct SystemStatus
{
    bool wifi_connected = false;
    bool udp_active = false;
    bool is_ready = false;
    bool led_connection_state = false;

    unsigned long last_command = 0;
    unsigned long last_debug = 0;
    unsigned long last_web_update = 0;
    unsigned long startup_time = 0;
    unsigned long last_led_blink = 0;

    int last_x = 0;
    int last_y = 0;
    int last_button = 0;
} status;

// Motor Status
struct MotorStatus
{
    int left_speed = 0;  // -255 to 255
    int right_speed = 0; // -255 to 255
} motors;

//=============== FUNCTION DECLARATIONS ===============

// Setup Functions
void setupMotors();
void setupWiFi();
void setupServer();
void setupLEDs();

// Motor Control
void stopMotors();
void setLeftMotor(int speed);
void setRightMotor(int speed);
void setMotorSpeeds(int x, int y, int button);

// Network & Communication
void handleUDPCommands();
void processCommand(char *command);
void updateWebClients();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

// Status & Debug
void printNetworkInfo();
void printDebugInfo();
void updateLEDs();

//=============== WEB INTERFACE ===============

// Web Interface HTML
const char *WEBPAGE = R"html(
<!DOCTYPE html>
<html>
<head>
        <title>RC Car Control Center!!</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        :root {
            --primary: #007bff;
            --success: #28a745;
            --danger: #dc3545;
            --light: #f8f9fa;
            --dark: #343a40;
        }
        
        body { 
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f0f0f0;
            color: var(--dark);
        }
        
        .container { 
                max-width: 1200px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 12px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        
        h1, h2 { 
            color: var(--dark);
            margin-top: 0;
        }
        
        .status {
            padding: 12px;
            margin: 10px 0;
            border-radius: 8px;
            font-weight: 500;
        }
        
        .connected { 
            background: #d4edda;
            color: #155724;
        }
        
        .disconnected { 
            background: #f8d7da;
            color: #721c24;
        }
            
            .main-controls {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 20px;
                margin: 20px 0;
            }
        
        .control-panel {
            background: var(--light);
            padding: 20px;
            border-radius: 12px;
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 15px;
            }
            
            .visualization-panel {
                background: var(--light);
                padding: 20px;
                border-radius: 12px;
                display: flex;
                flex-direction: column;
                gap: 20px;
            }
            
            .curve-container {
                background: white;
                padding: 20px;
                border-radius: 12px;
                box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }
            
            .curve-wrapper {
                position: relative;
                width: 100%;
                margin-bottom: 20px;
        }
        
        #joystick-zone {
            width: 240px;
            height: 240px;
            margin: 0 auto;
            background: #e9ecef;
            border-radius: 50%;
            position: relative;
            touch-action: none;
            border: 4px solid #dee2e6;
        }
        
        #joystick-knob {
            width: 80px;
            height: 80px;
            translate: -50% -50%;
            background: var(--primary);
            border-radius: 50%;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            cursor: pointer;
            transition: background 0.2s;
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
        }
        
        #joystick-knob:active {
            background: #0056b3;
        }
        
        .control-values {
            margin: 15px 0;
            font-family: monospace;
            font-size: 16px;
            background: white;
            padding: 10px;
            border-radius: 8px;
            display: inline-block;
        }
        
        #control-button {
            background: var(--danger);
            color: white;
            border: none;
            padding: 12px 30px;
            font-size: 18px;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.2s;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        #control-button:active {
            background: #c82333;
            transform: scale(0.98);
        }
        
        .value-box {
            background: #e9ecef;
            padding: 15px;
            margin: 10px 0;
            border-radius: 8px;
        }
        
        .motor-visual {
            width: 100%;
            height: 30px;
            background: #dee2e6;
            margin: 8px 0;
            border-radius: 15px;
            position: relative;
            overflow: hidden;
        }
        
        .motor-bar {
            height: 100%;
            background: var(--success);
            border-radius: 15px;
            transition: all 0.3s;
        }
        
        .negative {
            background: var(--danger);
        }
        
        @media (max-width: 600px) {
            body { padding: 10px; }
            .container { padding: 15px; }
            #joystick-zone { width: 200px; height: 200px; }
            #joystick-knob { width: 60px; height: 60px; }
        }
        
            .mode-selector {
                display: flex;
                gap: 10px;
            margin: 20px 0;
                justify-content: center;
            }
            
            .mode-button {
                padding: 12px 24px;
                border: 2px solid var(--primary);
                border-radius: 8px;
                background: white;
                color: var(--primary);
                cursor: pointer;
                font-weight: 600;
                transition: all 0.3s ease;
            }
            
            .mode-button.active {
                background: var(--primary);
                color: white;
            }
            
            .factor-controls {
            display: grid;
                grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
                margin-top: 20px;
            }
            
            .config-item {
                background: var(--light);
                padding: 12px;
                border-radius: 8px;
            }
            
            .config-item label {
                display: block;
                margin-bottom: 8px;
                font-weight: 500;
                font-size: 0.9em;
                color: var(--dark);
        }
        
        .slider {
            width: 100%;
                margin: 8px 0;
                height: 6px;
            background: #dee2e6;
                border-radius: 3px;
            -webkit-appearance: none;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
                width: 16px;
                height: 16px;
            background: var(--primary);
            border-radius: 50%;
            cursor: pointer;
                transition: all 0.2s ease;
        }
        
        .slider::-webkit-slider-thumb:hover {
                transform: scale(1.2);
                box-shadow: 0 0 10px rgba(0,123,255,0.3);
        }
        
        .value-display {
                display: inline-block;
                min-width: 40px;
                text-align: right;
                font-weight: 600;
                color: var(--primary);
            background: white;
            padding: 4px 8px;
            border-radius: 4px;
                font-size: 0.9em;
            }
            
            .button-group {
                display: flex;
                gap: 10px;
                justify-content: center;
                margin-top: 20px;
        }
        
        .config-button {
            padding: 12px 24px;
                border: none;
                border-radius: 8px;
            cursor: pointer;
            font-weight: 600;
                transition: all 0.3s ease;
            }
            
            .config-button.primary {
                background: var(--primary);
                color: white;
            }
            
            .config-button.secondary {
                background: var(--light);
                color: var(--dark);
        }
        
        .config-button:hover {
                transform: translateY(-2px);
                box-shadow: 0 2px 8px rgba(0,0,0,0.2);
            }
            
            .motor-status {
                margin-top: 20px;
            }
            
            .motor-bars {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 20px;
            }
            
            .current-point {
                position: absolute;
                width: 12px;
                height: 12px;
                background: #ff0000;
                border: 2px solid white;
                border-radius: 50%;
                transform: translate(-50%, -50%);
                pointer-events: none;
                box-shadow: 0 0 8px rgba(255,0,0,0.6);
                transition: all 0.1s ease;
                z-index: 10;
        }

        .input-source {
            margin-top: 10px;
            padding: 8px 12px;
            background: var(--light);
            border-radius: 8px;
            font-weight: 500;
        }

        .input-source.physical {
            background: var(--success);
            color: white;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>RC Car Control Center</h1>
        <div id="connectionStatus" class="status">Connecting...</div>
        
            <div class="main-controls">
        <div class="control-panel">
            <h2>Controls</h2>
            <div id="joystick-zone">
                <div id="joystick-knob"></div>
            </div>
            <div class="control-values">
                X: <span id="x-value">0</span>
                Y: <span id="y-value">0</span>
            </div>
            <button id="control-button">BOOST</button>
        </div>
        
                <div class="visualization-panel">
                    <div class="mode-selector">
                        <button class="mode-button" data-mode="0">Linear</button>
                        <button class="mode-button active" data-mode="1">Exponential</button>
                        <button class="mode-button" data-mode="2">Logarithmic</button>
                    </div>
                    <div class="curve-container">
                        <div class="curve-wrapper">
                            <canvas id="response-curve"></canvas>
                            <div id="current-point" class="current-point"></div>
                        </div>
                        <div class="factor-controls">
                            <div class="config-item">
                                <label for="exp-factor">Exponential Factor (0.1-50.0):</label>
                                <input type="range" id="exp-factor" min="1" max="500" value="20" class="slider" step="1">
                                <span class="value-display">2.0</span>
                            </div>
                            
                            <div class="config-item">
                                <label for="log-factor">Logarithmic Factor (0.1-10.0):</label>
                                <input type="range" id="log-factor" min="1" max="100" value="20" class="slider" step="1">
                                <span class="value-display">2.0</span>
                            </div>
                            
                            <div class="config-item">
                                <label for="boost-factor">Boost Factor (1.0-30.0):</label>
                                <input type="range" id="boost-factor" min="10" max="300" value="12" class="slider" step="1">
                                <span class="value-display">1.2</span>
                            </div>
                            
                            <div class="config-item">
                                <label for="deadzone">Deadzone (0-100):</label>
                                <input type="range" id="deadzone" min="0" max="100" value="8" class="slider">
                                <span class="value-display">8</span>
                            </div>
                            
                            <div class="config-item">
                                <label for="min-power">Minimum Power (0-255):</label>
                                <input type="range" id="min-power" min="0" max="255" value="45" class="slider">
                                <span class="value-display">45</span>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            
            <div class="motor-status">
        <h2>Motor Status</h2>
                <div class="motor-bars">
        <div class="value-box">
            <div>Left Motor: <span id="leftSpeed">0</span>%</div>
            <div class="motor-visual">
                <div id="leftBar" class="motor-bar" style="width: 50%; margin-left: 50%;"></div>
            </div>
        </div>
        <div class="value-box">
            <div>Right Motor: <span id="rightSpeed">0</span>%</div>
            <div class="motor-visual">
                <div id="rightBar" class="motor-bar" style="width: 50%; margin-left: 50%;"></div>
                        </div>
                    </div>
            </div>
        </div>
        
        <h2>Controller Input</h2>
        <div class="value-box">
            <div>Last Command: <span id="lastCommand">Waiting...</span></div>
            <div>Time Since Last Command: <span id="commandAge">0</span>ms</div>
        </div>
        
        <div class="config-panel">
                <h2>Control Configuration</h2>
                
                <div class="button-group">
                    <button id="apply-config" class="config-button primary">Apply Configuration</button>
                    <button id="reset-config" class="config-button secondary">Reset to Defaults</button>
            </div>
                
            <div id="config-status" class="config-status"></div>
        </div>

        <div class="input-source">
            Input Source: <span id="input-source">Web Interface</span>
        </div>
    </div>
    
    <script>
        // Update the WebSocket connection setup
        const ws = new WebSocket(`ws://${window.location.hostname}:81/`);

        ws.onopen = function() {
            console.log('WebSocket Connected');
            document.getElementById('connectionStatus').textContent = 'Connected';
            document.getElementById('connectionStatus').className = 'status connected';
        };

        ws.onclose = function() {
            console.log('WebSocket Disconnected');
            document.getElementById('connectionStatus').textContent = 'Disconnected';
            document.getElementById('connectionStatus').className = 'status disconnected';
            
            // Try to reconnect after a delay
            setTimeout(function() {
                ws = new WebSocket(`ws://${window.location.hostname}:81/`);
            }, 1000);
        };

        ws.onerror = function(error) {
            console.error('WebSocket Error:', error);
            document.getElementById('connectionStatus').textContent = 'Connection Error';
            document.getElementById('connectionStatus').className = 'status disconnected';
        };

        // Update the message handler
        ws.onmessage = function(event) {
            try {
                const data = JSON.parse(event.data);
                console.log('Received:', data);  // Debug log
                
                if (data.type === 'status_update') {
                    // Update input source indicator
                    const sourceElement = document.getElementById('input-source');
                    if (data.raw_input) {
                        sourceElement.textContent = 'Physical Controller';
                        sourceElement.parentElement.classList.add('physical');
                    } else {
                        sourceElement.textContent = 'Web Interface';
                        sourceElement.parentElement.classList.remove('physical');
                    }
                    
                    if (data.raw_input) {
                        // Update joystick visualization with physical controller values
                        currentX = data.last_x;
                        currentY = data.last_y;
                        buttonState = data.last_button;
                        
                        // Update joystick position visually
                        const radius = joystickZone.offsetWidth / 4;
                        const x = (currentX / 100) * radius;
                        const y = (-currentY / 100) * radius;
                        joystickKnob.style.transform = `translate(${x}px, ${y}px)`;
                        
                        // Update value displays
                        xValue.textContent = currentX;
                        yValue.textContent = currentY;
                        
                        // Update boost button visual state
                        controlButton.style.background = buttonState ? 'var(--danger)' : 'var(--primary)';
                        controlButton.classList.toggle('active', buttonState === 1);
                        
                        // Update motor status bars
                        updateMotorBar(data.left_speed, document.getElementById('leftBar'), document.getElementById('leftSpeed'));
                        updateMotorBar(data.right_speed, document.getElementById('rightBar'), document.getElementById('rightSpeed'));
                        
                        // Update last command display
                        document.getElementById('lastCommand').textContent = 
                            `X:${data.last_x} Y:${data.last_y} B:${data.last_button}`;
                        document.getElementById('commandAge').textContent = data.command_age;
                        
                        // Force curve redraw to show current position
                        drawResponseCurve();
                    }
                }
                else if (data.type === 'config_ack') {
                    // Update configuration status
                    const status = document.getElementById('config-status');
                    status.textContent = 'Configuration updated successfully';
                    status.className = 'config-status success';
                    setTimeout(() => {
                        status.textContent = '';
                    }, 2000);
                }
            } catch (e) {
                console.error('Error parsing WebSocket message:', e);
            }
        };
        
        // Control variables
        let isDragging = false;
        let currentX = 0;
        let currentY = 0;
        let buttonState = 0;
        let keyboardEnabled = true;  // Flag to enable/disable keyboard controls
        
        // Keyboard state
        const keyState = {
            up: false,
            down: false,
            left: false,
            right: false,
            space: false
        };
        
        // Add after the keyboard state
        let gamepadEnabled = true;  // Flag to enable/disable gamepad controls
        let gamepadLoopId = null;   // Store the gamepad polling loop ID
            let lastGamepadTimestamp = 0;
        
        // DOM Elements
        const joystickZone = document.getElementById('joystick-zone');
        const joystickKnob = document.getElementById('joystick-knob');
        const xValue = document.getElementById('x-value');
        const yValue = document.getElementById('y-value');
        const controlButton = document.getElementById('control-button');
        
        // Add keyboard control info to the UI
        const controlInfo = document.createElement('div');
        controlInfo.className = 'control-info';
        controlInfo.innerHTML = `
            <h3>Keyboard Controls</h3>
            <div class="keyboard-controls">
                <div>↑ Forward</div>
                <div>↓ Backward</div>
                <div>← Turn Left</div>
                <div>→ Turn Right</div>
                <div>Space Boost</div>
            </div>
        `;
        document.querySelector('.control-panel').appendChild(controlInfo);
        
        // Add gamepad control info to the UI
        const gamepadInfo = document.createElement('div');
        gamepadInfo.className = 'control-info';
        gamepadInfo.innerHTML = `
            <h3>Controller</h3>
            <div class="gamepad-controls">
                <div>Left Stick: Move</div>
                <div>A/X Button: Boost</div>
                <div id="gamepad-status">No Controller</div>
            </div>
        `;
        document.querySelector('.control-panel').appendChild(gamepadInfo);
        
        // Add styles for keyboard controls
        const style = document.createElement('style');
        style.textContent = `
            .control-info {
                margin-top: 20px;
                padding: 15px;
                background: white;
                border-radius: 8px;
                text-align: left;
            }
            .control-info h3 {
                margin: 0 0 10px 0;
                color: var(--dark);
            }
            .keyboard-controls {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
                gap: 8px;
            }
            .keyboard-controls div {
                padding: 8px;
                background: var(--light);
                border-radius: 4px;
                font-family: monospace;
                text-align: center;
            }
        `;
        document.head.appendChild(style);
        
        // Add styles for gamepad controls
        const gamepadStyles = `
            .gamepad-controls {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
                gap: 8px;
            }
            .gamepad-controls div {
                padding: 8px;
                background: var(--light);
                border-radius: 4px;
                font-family: monospace;
                text-align: center;
            }
            #gamepad-status {
                background: var(--danger);
                color: white;
            }
            #gamepad-status.connected {
                background: var(--success);
            }
        `;
        style.textContent += gamepadStyles;
        
        // Keyboard control functions
        function handleKeyDown(e) {
            if (!keyboardEnabled) return;
            
            switch(e.key) {
                case 'ArrowUp':
                    e.preventDefault();
                    keyState.up = true;
                    break;
                case 'ArrowDown':
                    e.preventDefault();
                    keyState.down = true;
                    break;
                case 'ArrowLeft':
                    e.preventDefault();
                    keyState.left = true;
                    break;
                case 'ArrowRight':
                    e.preventDefault();
                    keyState.right = true;
                    break;
                case ' ':  // Spacebar
                    e.preventDefault();
                    keyState.space = true;
                    buttonState = 1;
                    break;
            }
            updateKeyboardControl();
        }
        
        function handleKeyUp(e) {
            switch(e.key) {
                case 'ArrowUp':
                    keyState.up = false;
                    break;
                case 'ArrowDown':
                    keyState.down = false;
                    break;
                case 'ArrowLeft':
                    keyState.left = false;
                    break;
                case 'ArrowRight':
                    keyState.right = false;
                    break;
                case ' ':  // Spacebar
                    keyState.space = false;
                    buttonState = 0;
                    break;
            }
            updateKeyboardControl();
        }
        
        function updateKeyboardControl() {
            if (isDragging) return;  // Don't update if joystick is being used
            
            // Calculate Y value (forward/backward)
            if (keyState.up && !keyState.down) {
                currentY = 100;
            } else if (keyState.down && !keyState.up) {
                currentY = -100;
            } else {
                currentY = 0;
            }
            
            // Calculate X value (left/right)
            if (keyState.left && !keyState.right) {
                currentX = -100;
            } else if (keyState.right && !keyState.left) {
                currentX = 100;
            } else {
                currentX = 0;
            }
            
            // Update display
            xValue.textContent = currentX;
            yValue.textContent = currentY;
            
            // Update joystick visual position
            const radius = joystickZone.offsetWidth / 4;
            const x = (currentX / 100) * radius;
            const y = (-currentY / 100) * radius;
            joystickKnob.style.transform = `translate(${x}px, ${y}px)`;
            
            sendControlUpdate();
        }
        
        // Add keyboard event listeners
        document.addEventListener('keydown', handleKeyDown);
        document.addEventListener('keyup', handleKeyUp);
        
        // Disable keyboard controls when user starts using joystick
        joystickZone.addEventListener('mousedown', () => {
            keyboardEnabled = false;
        });
        
        joystickZone.addEventListener('touchstart', () => {
            keyboardEnabled = false;
        });
        
        // Re-enable keyboard controls when joystick is released
        document.addEventListener('mouseup', () => {
            setTimeout(() => { keyboardEnabled = true; }, 100);
        });
        
        document.addEventListener('touchend', () => {
            setTimeout(() => { keyboardEnabled = true; }, 100);
        });
        
        // Joystick Control Functions
        function startControl(e) {
            isDragging = true;
            updateJoystickPosition(e);
            e.preventDefault();
        }
        
        function stopControl() {
            isDragging = false;
            resetJoystick();
            // Reset keyboard controls too
            Object.keys(keyState).forEach(key => keyState[key] = false);
            currentX = 0;
            currentY = 0;
            buttonState = 0;
            updateKeyboardControl();
        }
        
        function updateJoystickPosition(e) {
            if (!isDragging) return;
            
            const rect = joystickZone.getBoundingClientRect();
            const centerX = rect.width / 2;
            const centerY = rect.height / 2;
            
            // Get touch or mouse position
            const clientX = e.type.includes('touch') ? e.touches[0].clientX : e.clientX;
            const clientY = e.type.includes('touch') ? e.touches[0].clientY : e.clientY;
            
            // Calculate position relative to center
            let x = clientX - rect.left - centerX;
            let y = clientY - rect.top - centerY;
            
            // Limit to circle
            const radius = rect.width / 2;
            const distance = Math.sqrt(x * x + y * y);
            if (distance > radius) {
                x = (x / distance) * radius;
                y = (y / distance) * radius;
            }
            
            // Update knob position
            joystickKnob.style.transform = `translate(${x}px, ${y}px)`;
            
            // Calculate control values (-100 to 100)
            currentX = Math.round((x / radius) * 100);
            currentY = Math.round((-y / radius) * 100); // Invert Y for intuitive control
            
            xValue.textContent = currentX;
            yValue.textContent = currentY;
            
            sendControlUpdate();
        }
        
        function resetJoystick() {
            joystickKnob.style.transform = 'translate(-50%, -50%)';
            currentX = 0;
            currentY = 0;
            xValue.textContent = currentX;
            yValue.textContent = currentY;
            sendControlUpdate();
        }
        
        function sendControlUpdate() {
            if (ws.readyState === WebSocket.OPEN) {
                const command = `${currentX},${currentY},${buttonState}`;
                ws.send(JSON.stringify({type: 'control', command: command}));
            }
        }
        
        // Event Listeners
        joystickZone.addEventListener('mousedown', startControl);
        joystickZone.addEventListener('touchstart', startControl);
        document.addEventListener('mousemove', updateJoystickPosition);
        document.addEventListener('touchmove', updateJoystickPosition);
        document.addEventListener('mouseup', stopControl);
        document.addEventListener('touchend', stopControl);
        
        controlButton.addEventListener('mousedown', () => {
            buttonState = 1;
            sendControlUpdate();
        });
        
        controlButton.addEventListener('mouseup', () => {
            buttonState = 0;
            sendControlUpdate();
        });
        
        // Touch events for mobile
        controlButton.addEventListener('touchstart', (e) => {
            e.preventDefault();
            buttonState = 1;
            sendControlUpdate();
        });
        
        controlButton.addEventListener('touchend', (e) => {
            e.preventDefault();
            buttonState = 0;
            sendControlUpdate();
        });
        
        // Update motor display
        function updateMotorBar(value, barElement, textElement) {
            const percent = Math.abs(value) * 100 / 255;
            barElement.style.width = percent + '%';
            barElement.style.marginLeft = value >= 0 ? '50%' : (50 - percent) + '%';
            barElement.className = 'motor-bar ' + (value < 0 ? 'negative' : '');
            textElement.textContent = Math.round(value * 100 / 255);
        }
        
        // Gamepad handling functions
        function handleGamepad() {
            const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
            let gamepad = null;
            
            // Find the first active gamepad
            for (let i = 0; i < gamepads.length; i++) {
                if (gamepads[i] && gamepads[i].connected) {
                    gamepad = gamepads[i];
                    // Only process if gamepad state has changed
                    if (gamepad.timestamp === lastGamepadTimestamp) {
                        return;
                    }
                    lastGamepadTimestamp = gamepad.timestamp;
                    break;
                }
            }
            
            if (gamepad) {
                // Update status
                const statusEl = document.getElementById('gamepad-status');
                statusEl.textContent = 'Connected: ' + gamepad.id.substring(0, 20);
                statusEl.className = 'connected';
                
                // Get trigger values (usually axes[2] for LT and axes[5] for RT)
                let leftTrigger = 0;
                let rightTrigger = 0;
                
                // Different gamepads might use different mappings
                if (typeof gamepad.axes[2] !== 'undefined') {
                    leftTrigger = (gamepad.axes[2] + 1) / 2; // Convert -1,1 to 0,1
                }
                if (typeof gamepad.axes[5] !== 'undefined') {
                    rightTrigger = (gamepad.axes[5] + 1) / 2; // Convert -1,1 to 0,1
                }
                
                // Get joystick values
                let leftStickY = 0;
                let rightStickY = 0;
                
                // Left stick Y (usually axes[1])
                if (typeof gamepad.axes[1] !== 'undefined') {
                    leftStickY = -gamepad.axes[1]; // Invert for intuitive control
                }
                
                // Right stick Y (usually axes[3])
                if (typeof gamepad.axes[3] !== 'undefined') {
                    rightStickY = -gamepad.axes[3]; // Invert for intuitive control
                }
                
                // Apply deadzone to sticks
                const deadzone = 0.1;
                leftStickY = Math.abs(leftStickY) < deadzone ? 0 : leftStickY;
                rightStickY = Math.abs(rightStickY) < deadzone ? 0 : rightStickY;
                
                // Calculate motor speeds based on triggers and sticks
                let leftMotorSpeed = 0;
                let rightMotorSpeed = 0;
                
                // Trigger control (forward/backward)
                const triggerDiff = rightTrigger - leftTrigger;
                
                // Combine trigger and stick inputs
                leftMotorSpeed = (triggerDiff + leftStickY) * 100;
                rightMotorSpeed = (triggerDiff + rightStickY) * 100;
                
                // Constrain values to -100 to 100
                leftMotorSpeed = Math.max(-100, Math.min(100, leftMotorSpeed));
                rightMotorSpeed = Math.max(-100, Math.min(100, rightMotorSpeed));
                
                // Check D-pad buttons (usually buttons 12-15)
                const dpadUp = gamepad.buttons[12] && gamepad.buttons[12].pressed;
                const dpadDown = gamepad.buttons[13] && gamepad.buttons[13].pressed;
                const dpadLeft = gamepad.buttons[14] && gamepad.buttons[14].pressed;
                const dpadRight = gamepad.buttons[15] && gamepad.buttons[15].pressed;
                
                // Apply D-pad controls if pressed
                if (dpadUp) {
                    leftMotorSpeed = 100;
                    rightMotorSpeed = 100;
                } else if (dpadDown) {
                    leftMotorSpeed = -100;
                    rightMotorSpeed = -100;
                } else if (dpadLeft) {
                    leftMotorSpeed = -100;
                    rightMotorSpeed = 100;
                } else if (dpadRight) {
                    leftMotorSpeed = 100;
                    rightMotorSpeed = -100;
                }
                
                // Check boost button (usually A button is 0)
                buttonState = gamepad.buttons[0] && gamepad.buttons[0].pressed ? 1 : 0;
                
                // Update current values for the interface
                currentX = Math.round((rightMotorSpeed - leftMotorSpeed) / 2);
                currentY = Math.round((rightMotorSpeed + leftMotorSpeed) / 2);
                
                // Update visual feedback
                const radius = joystickZone.offsetWidth / 4;
                const visualX = (currentX / 100) * radius;
                const visualY = (currentY / 100) * radius;
                joystickKnob.style.transform = `translate(${visualX}px, ${visualY}px)`;
                
                // Update display values
                xValue.textContent = currentX;
                yValue.textContent = currentY;
                controlButton.style.background = buttonState ? 'var(--danger)' : 'var(--primary)';
                
                // Send command
                sendControlUpdate();
                
                if (DEBUG) {
                    console.log(`Gamepad: LM:${leftMotorSpeed} RM:${rightMotorSpeed} Boost:${buttonState}`);
                    console.log(`Triggers: LT:${leftTrigger.toFixed(2)} RT:${rightTrigger.toFixed(2)}`);
                    console.log(`Sticks: LS:${leftStickY.toFixed(2)} RS:${rightStickY.toFixed(2)}`);
                }
            }
        }
        
            // Start gamepad polling with error handling
        function startGamepadPolling() {
                try {
            if (!gamepadLoopId) {
                        console.log('Starting gamepad polling...');
                        gamepadLoopId = setInterval(handleGamepad, 50);  // 20Hz update rate
                        handleGamepad();  // Initial check
                    }
                } catch (error) {
                    console.error('Error starting gamepad polling:', error);
            }
        }
        
        // Stop gamepad polling
        function stopGamepadPolling() {
            if (gamepadLoopId) {
                clearInterval(gamepadLoopId);
                gamepadLoopId = null;
            }
        }
        
            // Improved gamepad connection handlers
        window.addEventListener("gamepadconnected", function(e) {
                console.log("Gamepad connected:", e.gamepad);
                const statusEl = document.getElementById('gamepad-status');
                statusEl.textContent = 'Connected: ' + e.gamepad.id.substring(0, 20);
                statusEl.className = 'connected';
            startGamepadPolling();
        });
        
        window.addEventListener("gamepaddisconnected", function(e) {
                console.log("Gamepad disconnected:", e.gamepad);
                const statusEl = document.getElementById('gamepad-status');
                statusEl.textContent = 'No Controller';
                statusEl.className = '';
                stopGamepadPolling();
            });

            // Initial gamepad check
            function checkGamepads() {
                const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
                for (let i = 0; i < gamepads.length; i++) {
                    if (gamepads[i] && gamepads[i].connected) {
                        console.log('Found connected gamepad:', gamepads[i]);
                        const statusEl = document.getElementById('gamepad-status');
                        statusEl.textContent = 'Connected: ' + gamepads[i].id.substring(0, 20);
                        statusEl.className = 'connected';
                        startGamepadPolling();
                        break;
                    }
                }
            }

            // Check for gamepads when page loads
            window.addEventListener('load', function() {
                console.log('Checking for gamepads...');
                setTimeout(checkGamepads, 1000);  // Check after a short delay
            });

            // Add debug button to force gamepad check
            const debugButton = document.createElement('button');
            debugButton.textContent = 'Check Gamepad';
            debugButton.className = 'config-button';
            debugButton.style.marginTop = '10px';
            debugButton.onclick = checkGamepads;
            document.querySelector('.gamepad-controls').appendChild(debugButton);
            
            // Configuration and UI Elements
            const CONFIG = {
                storageKey: 'rcCarConfig',
                updateDelay: 100,
                sliders: {
                    'exp-factor': { min: 1, max: 500, scale: 0.1, default: 20 },  // 0.1 to 50.0
                    'log-factor': { min: 1, max: 100, scale: 0.1, default: 20 },  // 0.1 to 10.0
                    'boost-factor': { min: 10, max: 300, scale: 0.1, default: 12 }, // 1.0 to 30.0
                    'deadzone': { min: 0, max: 100, scale: 1, default: 8 },       // 0 to 100
                    'min-power': { min: 0, max: 255, scale: 1, default: 45 }      // 0 to 255
                },
                modes: {
                    LINEAR: 0,
                    EXPONENTIAL: 1,
                    LOGARITHMIC: 2
                }
            };

            let currentMode = CONFIG.modes.EXPONENTIAL;
            let configUpdateTimeout = null;

            // Initialize UI
            function initializeUI() {
                // Mode selection setup
                const modeButtons = document.querySelectorAll('.mode-button');
                modeButtons.forEach(button => {
                    button.addEventListener('click', () => {
                        modeButtons.forEach(btn => btn.classList.remove('active'));
                        button.classList.add('active');
                        currentMode = parseInt(button.dataset.mode);
                        drawResponseCurve();
                        sendConfigUpdate();
                        saveConfig();
                    });
                });

                // Initialize sliders
                Object.entries(CONFIG.sliders).forEach(([id, config]) => {
                    const slider = document.getElementById(id);
            const display = slider.nextElementSibling;
                    
                    // Set initial values
                    slider.value = config.default;
                    display.textContent = (config.default * config.scale).toFixed(config.scale < 1 ? 1 : 0);
                    
                    // Add input event listener for real-time updates
            slider.addEventListener('input', () => {
                        const value = parseFloat(slider.value);
                        const scaledValue = (value * config.scale).toFixed(config.scale < 1 ? 1 : 0);
                        display.textContent = scaledValue;
                        
                        // Update the response curve immediately
                        drawResponseCurve();
                        
                        // Debounce the configuration update
                        if (configUpdateTimeout) {
                            clearTimeout(configUpdateTimeout);
                        }
                        
                        configUpdateTimeout = setTimeout(() => {
                            sendConfigUpdate();
                            saveConfig();
                        }, CONFIG.updateDelay);
            });
        });
        
                // Add window resize handler with debouncing
                const resizeObserver = new ResizeObserver(debounce(() => {
                    const canvas = document.getElementById('response-curve');
                    const container = canvas.parentElement;
                    const rect = container.getBoundingClientRect();
                    
                    canvas.width = rect.width;
                    canvas.height = Math.min(rect.width * 0.6, 300);
                    
                    drawResponseCurve();
                }, 100));

                // Observe the curve container for size changes
                resizeObserver.observe(document.querySelector('.curve-wrapper'));

                // Load saved configuration
                loadConfig();
            }

            // Configuration update function
            function sendConfigUpdate() {
                if (ws.readyState === WebSocket.OPEN) {
            const config = {
                type: 'config',
                        min_power: parseInt(document.getElementById('min-power').value),
                        exp_factor: parseFloat(document.getElementById('exp-factor').value) / 10,
                        log_factor: parseFloat(document.getElementById('log-factor').value) / 10,
                        boost_factor: parseFloat(document.getElementById('boost-factor').value) / 10,
                        deadzone: parseInt(document.getElementById('deadzone').value),
                        mode: currentMode
            };
            
            ws.send(JSON.stringify(config));
                    showConfigStatus('Configuration updated', 'success');
                }
            }
            
            // Status message handling
            function showConfigStatus(message, type) {
            const status = document.getElementById('config-status');
                status.textContent = message;
                status.className = `config-status ${type}`;
                
                // Add animation
                status.style.animation = 'none';
                status.offsetHeight; // Trigger reflow
                status.style.animation = 'fadeInOut 2s ease-in-out';
            }

            // Response curve drawing
            function drawResponseCurve() {
                const canvas = document.getElementById('response-curve');
                const ctx = canvas.getContext('2d');
                
                // Set canvas size based on container
                const container = canvas.parentElement;
                canvas.width = container.clientWidth;
                canvas.height = Math.min(container.clientWidth * 0.6, 300);
                
                const width = canvas.width;
                const height = canvas.height;
                
                // Clear and set background
                ctx.fillStyle = '#f8f9fa';
                ctx.fillRect(0, 0, width, height);
                
                // Draw grid and labels
                drawGrid(ctx, width, height);
                
                // Draw curve
                drawCurve(ctx, width, height);
            }

            function drawGrid(ctx, width, height) {
                // Draw grid lines
                ctx.strokeStyle = '#e0e0e0';
                ctx.lineWidth = 1;
                ctx.textAlign = 'right';
                ctx.font = '12px Arial';
                ctx.fillStyle = '#666';
                
                // Draw deadzone area first
                const deadzone = parseInt(document.getElementById('deadzone').value);
                const deadzoneWidth = (deadzone / 100) * (width / 2);
                
                // Draw vertical deadzone areas
                ctx.fillStyle = 'rgba(255, 0, 0, 0.1)';
                ctx.fillRect(width/2 - deadzoneWidth, 0, deadzoneWidth * 2, height);
                
                // Draw horizontal deadzone areas
                ctx.fillStyle = 'rgba(255, 0, 0, 0.1)';
                ctx.fillRect(0, height/2 - deadzoneWidth, width, deadzoneWidth * 2);
                
                // Draw deadzone borders
                ctx.strokeStyle = 'rgba(255, 0, 0, 0.5)';
                ctx.lineWidth = 1;
                ctx.beginPath();
                // Vertical lines
                ctx.moveTo(width/2 - deadzoneWidth, 0);
                ctx.lineTo(width/2 - deadzoneWidth, height);
                ctx.moveTo(width/2 + deadzoneWidth, 0);
                ctx.lineTo(width/2 + deadzoneWidth, height);
                // Horizontal lines
                ctx.moveTo(0, height/2 - deadzoneWidth);
                ctx.lineTo(width, height/2 - deadzoneWidth);
                ctx.moveTo(0, height/2 + deadzoneWidth);
                ctx.lineTo(width, height/2 + deadzoneWidth);
                ctx.stroke();
                
                // Add deadzone label
                ctx.fillStyle = 'rgba(255, 0, 0, 0.7)';
                ctx.textAlign = 'center';
                ctx.font = '12px Arial';
                ctx.fillText(`Deadzone: ${deadzone}%`, width/2, height - 5);
                
                // Draw grid lines
                ctx.strokeStyle = '#e0e0e0';
                ctx.lineWidth = 1;
                for (let i = 0; i <= 10; i++) {
                    const x = (width * i) / 10;
                    const y = (height * i) / 10;
                    
                    ctx.beginPath();
                    ctx.moveTo(x, 0);
                    ctx.lineTo(x, height);
                    ctx.moveTo(0, y);
                    ctx.lineTo(width, y);
                    ctx.stroke();
                    
                    // Labels
                    if (i % 2 === 0) {
                        ctx.fillStyle = '#666';
                        ctx.textAlign = 'right';
                        ctx.fillText(((i - 5) * 20).toString(), x - 3, height - 3);
                        ctx.fillText(((5 - i) * 20).toString(), width - 3, y + 12);
                    }
                }
                
                // Draw axes
                ctx.strokeStyle = '#000';
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(width/2, 0);
                ctx.lineTo(width/2, height);
                ctx.moveTo(0, height/2);
                ctx.lineTo(width, height/2);
                ctx.stroke();
            }

            function drawCurve(ctx, width, height) {
                ctx.clearRect(0, 0, width, height);
                drawGrid(ctx, width, height);
                
                const expFactor = parseFloat(document.getElementById('exp-factor').value) / 10;
                const logFactor = parseFloat(document.getElementById('log-factor').value) / 10;
                const boostFactor = parseFloat(document.getElementById('boost-factor').value) / 10;
                const deadzone = parseInt(document.getElementById('deadzone').value);
                
                // Draw X axis curve
                ctx.beginPath();
                const points = 200;
                for(let i = 0; i <= points; i++) {
                    const input = (i / points) * 2 - 1;
                    let output = calculateOutput(input, expFactor, logFactor, currentMode);
                    
                    // Apply deadzone
                    if (Math.abs(input) < deadzone/100) {
                        output = 0;
                    }
                    
                    // Apply boost if active
                    if(Math.abs(input) > 0.7 && buttonState === 1) {
                        const boostAmount = (Math.abs(input) - 0.7) / 0.3;
                        const boost = Math.pow(boostAmount, 2) * (boostFactor - 1.0);
                        output *= (1.0 + boost);
                    }
                    
                    const x = (input + 1) * width / 2;
                    const y = height/2 - (output * height/2);
                    
                    if(i === 0) ctx.moveTo(x, y);
                    else ctx.lineTo(x, y);
                }
                
                // Draw X curve with blue gradient
                const xGradient = ctx.createLinearGradient(0, 0, width, 0);
                xGradient.addColorStop(0, 'rgba(0, 123, 255, 0.8)');
                xGradient.addColorStop(0.5, 'rgba(0, 123, 255, 0.4)');
                xGradient.addColorStop(1, 'rgba(0, 123, 255, 0.8)');
                ctx.strokeStyle = xGradient;
                ctx.lineWidth = 3;
                ctx.stroke();
                
                // Draw Y axis curve
                ctx.beginPath();
                for(let i = 0; i <= points; i++) {
                    const input = (i / points) * 2 - 1;
                    let output = calculateOutput(input, expFactor, logFactor, currentMode);
                    
                    // Apply deadzone
                    if (Math.abs(input) < deadzone/100) {
                        output = 0;
                    }
                    
                    // Apply boost if active
                    if(Math.abs(input) > 0.7 && buttonState === 1) {
                        const boostAmount = (Math.abs(input) - 0.7) / 0.3;
                        const boost = Math.pow(boostAmount, 2) * (boostFactor - 1.0);
                        output *= (1.0 + boost);
                    }
                    
                    const y = (input + 1) * height / 2;
                    const x = width/2 + (output * width/2);
                    
                    if(i === 0) ctx.moveTo(x, y);
                    else ctx.lineTo(x, y);
                }
                
                // Draw Y curve with green gradient
                const yGradient = ctx.createLinearGradient(0, 0, 0, height);
                yGradient.addColorStop(0, 'rgba(40, 167, 69, 0.8)');
                yGradient.addColorStop(0.5, 'rgba(40, 167, 69, 0.4)');
                yGradient.addColorStop(1, 'rgba(40, 167, 69, 0.8)');
                ctx.strokeStyle = yGradient;
                ctx.lineWidth = 3;
                ctx.stroke();
                
                // Plot current joystick position if active
                if (currentX !== 0 || currentY !== 0) {
                    const inputX = currentX / 100;
                    const inputY = currentY / 100;
                    
                    let outputX = calculateOutput(inputX, expFactor, logFactor, currentMode);
                    let outputY = calculateOutput(inputY, expFactor, logFactor, currentMode);
                    
                    // Apply deadzone
                    if (Math.abs(inputX) < deadzone/100) outputX = 0;
                    if (Math.abs(inputY) < deadzone/100) outputY = 0;
                    
                    // Apply boost
                    if(Math.abs(inputX) > 0.7 && buttonState === 1) {
                        const boostAmount = (Math.abs(inputX) - 0.7) / 0.3;
                        const boost = Math.pow(boostAmount, 2) * (boostFactor - 1.0);
                        outputX *= (1.0 + boost);
                    }
                    if(Math.abs(inputY) > 0.7 && buttonState === 1) {
                        const boostAmount = (Math.abs(inputY) - 0.7) / 0.3;
                        const boost = Math.pow(boostAmount, 2) * (boostFactor - 1.0);
                        outputY *= (1.0 + boost);
                    }
                    
                    const x = (inputX + 1) * width / 2;
                    const y = (inputY + 1) * height / 2;
                    
                    // Draw position marker with glow
                    ctx.save();
                    ctx.beginPath();
                    ctx.arc(x, y, 12, 0, Math.PI * 2);
                    ctx.fillStyle = buttonState === 1 ? 'rgba(220, 53, 69, 0.3)' : 'rgba(0, 123, 255, 0.3)';
                    ctx.fill();
                    
                    ctx.beginPath();
                    ctx.arc(x, y, 6, 0, Math.PI * 2);
                    ctx.fillStyle = buttonState === 1 ? '#dc3545' : '#007bff';
                    ctx.fill();
                    ctx.strokeStyle = 'white';
                    ctx.lineWidth = 2;
                    ctx.stroke();
                    
                    // Show values with background
                    ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
                    ctx.fillRect(5, 5, 150, 110);
                    
                    ctx.fillStyle = '#000';
                    ctx.textAlign = 'left';
                    ctx.font = '12px Arial';
                    ctx.fillText(`Input X: ${currentX}`, 10, 20);
                    ctx.fillText(`Output X: ${Math.round(outputX * 100)}`, 10, 40);
                    ctx.fillText(`Input Y: ${currentY}`, 10, 60);
                    ctx.fillText(`Output Y: ${Math.round(outputY * 100)}`, 10, 80);
                    
                    if (buttonState === 1) {
                        ctx.fillStyle = '#dc3545';
                        ctx.fillText('BOOST ACTIVE', 10, 100);
                    }
                    
                    ctx.restore();
                }
                
                // Add legend
                ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
                ctx.fillRect(width - 100, 5, 95, 50);
                
                ctx.fillStyle = 'rgba(0, 123, 255, 0.8)';
                ctx.fillRect(width - 90, 15, 20, 3);
                ctx.fillStyle = '#000';
                ctx.fillText('X Axis', width - 60, 20);
                
                ctx.fillStyle = 'rgba(40, 167, 69, 0.8)';
                ctx.fillRect(width - 90, 35, 20, 3);
                ctx.fillStyle = '#000';
                ctx.fillText('Y Axis', width - 60, 40);
            }

            function calculateOutput(input, expFactor, logFactor, mode) {
                switch(mode) {
                    case CONFIG.modes.LINEAR:
                        return input;
                    case CONFIG.modes.EXPONENTIAL:
                        return Math.pow(Math.abs(input), expFactor) * Math.sign(input);
                    case CONFIG.modes.LOGARITHMIC:
                        return Math.log(1 + Math.abs(input) * logFactor) / Math.log(1 + logFactor) * Math.sign(input);
                    default:
                        return input;
                }
            }

            // Add these functions for configuration persistence
            function saveConfig() {
                const config = {
                    mode: currentMode,
                    values: {}
                };
                
                // Save all slider values
                Object.entries(CONFIG.sliders).forEach(([id, _]) => {
                    config.values[id] = parseFloat(document.getElementById(id).value);
                });
                
                localStorage.setItem(CONFIG.storageKey, JSON.stringify(config));
            }

            function loadConfig() {
                const saved = localStorage.getItem(CONFIG.storageKey);
                if (saved) {
                    try {
                        const config = JSON.parse(saved);
                        
                        // Restore mode
                        currentMode = config.mode;
                        document.querySelectorAll('.mode-button').forEach(button => {
                            if (parseInt(button.dataset.mode) === currentMode) {
                                button.classList.add('active');
                            } else {
                                button.classList.remove('active');
                            }
                        });
                        
                        // Restore slider values
                        Object.entries(config.values).forEach(([id, value]) => {
                            const slider = document.getElementById(id);
                            const display = slider.nextElementSibling;
                            if (slider && display) {
                                slider.value = value;
                                const scale = CONFIG.sliders[id].scale;
                                display.textContent = (value * scale).toFixed(scale < 1 ? 1 : 0);
                            }
                        });
                        
                        // Update curve with loaded values
                        drawResponseCurve();
                        // Send configuration to the car
                        sendConfigUpdate();
                    } catch (e) {
                        console.error('Error loading saved configuration:', e);
                    }
                }
            }

            // Add reset handler
            document.getElementById('reset-config').addEventListener('click', () => {
                // Reset all sliders to defaults
                Object.entries(CONFIG.sliders).forEach(([id, config]) => {
                    const slider = document.getElementById(id);
                    const display = slider.nextElementSibling;
                    slider.value = config.default;
                    display.textContent = (config.default * config.scale).toFixed(config.scale < 1 ? 1 : 0);
                });
                
                // Reset mode to default
                currentMode = CONFIG.modes.EXPONENTIAL;
                document.querySelectorAll('.mode-button').forEach(button => {
                    button.classList.toggle('active', parseInt(button.dataset.mode) === currentMode);
                });
                
                // Update curve and send to car
                drawResponseCurve();
                sendConfigUpdate();
                
                // Clear saved configuration
                localStorage.removeItem(CONFIG.storageKey);
            });

            // Add debounce utility function
            function debounce(func, wait) {
                let timeout;
                return function executedFunction(...args) {
                    const later = () => {
                        clearTimeout(timeout);
                        func(...args);
                    };
                    clearTimeout(timeout);
                    timeout = setTimeout(later, wait);
                };
            }

            // Initialize everything when the document is ready
            document.addEventListener('DOMContentLoaded', initializeUI);

            // Add animation frame request
            let animationFrameId = null;

            function startCurveAnimation() {
                function animate() {
                    drawResponseCurve();
                    animationFrameId = requestAnimationFrame(animate);
                }
                animate();
            }

            function stopCurveAnimation() {
                if (animationFrameId) {
                    cancelAnimationFrame(animationFrameId);
                    animationFrameId = null;
                }
            }

            // Start animation when page loads
            document.addEventListener('DOMContentLoaded', () => {
                initializeUI();
                startCurveAnimation();
            });
    </script>
</body>
</html>
)html";

//=============== SETUP & LOOP ===============

// Add high-performance timer setup
hw_timer_t *controlTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Motor state tracking
volatile int prev_left = 0;
volatile int prev_right = 0;

// IRAM_ATTR ensures the function runs from RAM
void IRAM_ATTR updateMotors()
{
    portENTER_CRITICAL_ISR(&timerMux);
    // Only update if there's a change in speed
    if (motors.left_speed != prev_left)
    {
        setLeftMotor(motors.left_speed);
        prev_left = motors.left_speed;
    }
    if (motors.right_speed != prev_right)
    {
        setRightMotor(motors.right_speed);
        prev_right = motors.right_speed;
    }
    portEXIT_CRITICAL_ISR(&timerMux);
}

// Timer interrupt handler
void IRAM_ATTR onTimer()
{
    updateMotors();
}

// Update setup function with correct timer initialization
void setup()
{
    Serial.begin(115200);
    Serial.println("\n=== RC Car Initializing ===");

    // Initialize EEPROM
    if (!EEPROM.begin(EEPROMConfig::EEPROM_SIZE))
    {
        Serial.println("[EEPROM] Failed to initialize EEPROM!");
    }
    else
    {
        Serial.println("[EEPROM] Initialized successfully");
        // Load saved configuration
        if (!motor_config.loadFromEEPROM())
        {
            Serial.println("[EEPROM] Using default configuration");
            motor_config.saveToEEPROM(); // Save defaults for next boot
        }
    }

    // Set CPU frequency to 240MHz
    setCpuFrequencyMhz(240);
    Serial.printf("[CPU] Running at %dMHz\n", getCpuFrequencyMhz());

    // Initialize timer for motor control (500Hz)
    controlTimer = timerBegin(500); // 500Hz frequency
    if (controlTimer)
    {
        timerAttachInterrupt(controlTimer, &onTimer);
        timerStart(controlTimer);
        Serial.println("[Timer] High-performance timer initialized at 500Hz");
    }
    else
    {
        Serial.println("[Timer] Failed to initialize timer!");
    }

    setupLEDs();
    setupMotors();
    setupWiFi();
    setupServer();

    status.startup_time = millis();
    printNetworkInfo();

    Serial.println("=== Initialization Complete ===\n");
}

void loop()
{
    // Update system status
    status.wifi_connected = (WiFi.status() == WL_CONNECTED);

    // Handle communication
    webSocket.loop();
    server.handleClient();
    handleUDPCommands();

    // Update status indicators
    updateLEDs();

    // Safety timeout check
    if (millis() - status.last_command > timing.command_timeout)
    {
        if (motors.left_speed != 0 || motors.right_speed != 0)
        {
            stopMotors();
        }
    }

    // Periodic updates
    if (millis() - status.last_debug >= timing.debug_interval)
    {
        printDebugInfo();
        status.last_debug = millis();
    }

    if (millis() - status.last_web_update >= timing.web_update_interval)
    {
        updateWebClients();
        status.last_web_update = millis();
    }
}

//=============== MOTOR CONTROL ===============

void setupMotors()
{
    // Configure motor pins
    pinMode(pins.ena, OUTPUT);
    pinMode(pins.in1, OUTPUT);
    pinMode(pins.in2, OUTPUT);
    pinMode(pins.enb, OUTPUT);
    pinMode(pins.in3, OUTPUT);
    pinMode(pins.in4, OUTPUT);

    // Set PWM frequency for smoother motor control
    analogWriteFrequency(pins.ena, 20000); // 20kHz
    analogWriteFrequency(pins.enb, 20000); // 20kHz

    // Initialize motors to stopped state
    stopMotors();

    Serial.println("[Motors] Setup complete");
}

void stopMotors()
{
    // Stop left motor
    digitalWrite(pins.in1, LOW);
    digitalWrite(pins.in2, LOW);
    analogWrite(pins.ena, 0);

    // Stop right motor
    digitalWrite(pins.in3, LOW);
    digitalWrite(pins.in4, LOW);
    analogWrite(pins.enb, 0);

    motors.left_speed = 0;
    motors.right_speed = 0;
}

/**
 * Apply control mapping based on selected mode for a single axis
 * @param input Raw input value (-100 to 100)
 * @param isYAxis Whether this is Y axis (for different curve parameters if needed)
 * @return Mapped motor speed (-255 to 255)
 */
int mapAxisToMotorSpeed(int input, bool isYAxis)
{
    if (abs(input) < motor_config.deadzone)
        return 0;

    // Get sign and normalize to 0-1 range
    int sign = input > 0 ? 1 : -1;
    float normalized = abs(input) / 100.0f;
    float mapped = 0.0f;

    // Apply selected control mode
    switch (motor_config.mode)
    {
    case MotorConfig::LINEAR:
        mapped = normalized;
        break;

    case MotorConfig::EXPONENTIAL:
        mapped = pow(normalized, motor_config.exp_factor);
        break;

    case MotorConfig::LOGARITHMIC:
        mapped = log(1 + (normalized * motor_config.log_factor)) / log(1 + motor_config.log_factor);
        break;
    }

    // Apply boost if active and input is high enough
    if (normalized > 0.7f)
    {
        float boostAmount = (normalized - 0.7f) / 0.3f;
        float boost = pow(boostAmount, 2) * (motor_config.boost_factor - 1.0f);
        mapped *= (1.0f + boost);
    }

    // Scale to motor range
    int speed = round(mapped * motor_config.max_speed);

    // Apply minimum power threshold when moving
    if (speed > 0)
    {
        speed = max(speed, motor_config.min_power);
    }
    else if (speed < 0)
    {
        speed = min(speed, -motor_config.min_power);
    }

    // Apply sign and constrain
    speed *= sign;
    return constrain(speed, motor_config.min_speed, motor_config.max_speed);
}

/**
 * Set motor speeds with improved precision control and better rotation
 * @param x Raw X input (-100 to 100)
 * @param y Raw Y input (-100 to 100)
 * @param button Boost button state
 */
void setMotorSpeeds(int x, int y, int button)
{
    // Map both axes through the control curve for smooth response
    int mappedX = mapAxisToMotorSpeed(x, false);
    int mappedY = mapAxisToMotorSpeed(y, true);

    // Calculate final motor speeds using the mapped values
    float leftSpeed = 0;
    float rightSpeed = 0;

    // Normalize mapped values to -1.0 to 1.0 for easier calculations
    float normalizedX = mappedX / (float)motor_config.max_speed;
    float normalizedY = mappedY / (float)motor_config.max_speed;

    if (button == 1 && abs(normalizedX) > 0.1f)
    {
        // Spin in place mode with smooth acceleration
        float spinPower = pow(abs(normalizedX), 1.5f); // Exponential for better control
        leftSpeed = -mappedX * spinPower;
        rightSpeed = mappedX * spinPower;
    }
    else
    {
        // Normal driving mode with improved turning
        if (abs(mappedY) < (motor_config.max_speed * 0.1f))
        {
            // Pure turning when almost no forward/backward input
            float turnPower = pow(abs(normalizedX), 1.8f); // More exponential for finer control
            leftSpeed = mappedX * turnPower;
            rightSpeed = -mappedX * turnPower;
        }
        else
        {
            // Mixed mode - forward/backward with turning
            leftSpeed = mappedY;
            rightSpeed = mappedY;

            // Apply proportional turning reduction
            if (abs(normalizedX) > 0.05f)
            { // Small deadzone for turning
                // Calculate turn influence (0.0 to 1.0) with exponential curve
                float turnInfluence = pow(abs(normalizedX), 1.5f);

                // Apply turning reduction with direction
                if (normalizedX > 0)
                {
                    // Turning right: reduce right motor proportionally
                    rightSpeed *= (1.0f - turnInfluence);
                    // Optional: Slightly increase left motor for better turning
                    leftSpeed *= (1.0f + (turnInfluence * 0.2f));
                }
                else
                {
                    // Turning left: reduce left motor proportionally
                    leftSpeed *= (1.0f - turnInfluence);
                    // Optional: Slightly increase right motor for better turning
                    rightSpeed *= (1.0f + (turnInfluence * 0.2f));
                }
            }
        }
    }

    // Apply boost if active
    if (button == 1)
    {
        float boostMultiplier = motor_config.boost_factor;
        leftSpeed *= boostMultiplier;
        rightSpeed *= boostMultiplier;
    }

    // Constrain final speeds
    int finalLeftSpeed = constrain(round(leftSpeed), motor_config.min_speed, motor_config.max_speed);
    int finalRightSpeed = constrain(round(rightSpeed), motor_config.min_speed, motor_config.max_speed);

    // Apply minimum power threshold when moving
    if (abs(finalLeftSpeed) > 0)
    {
        finalLeftSpeed = (finalLeftSpeed > 0) ? max(finalLeftSpeed, motor_config.min_power) : min(finalLeftSpeed, -motor_config.min_power);
    }
    if (abs(finalRightSpeed) > 0)
    {
        finalRightSpeed = (finalRightSpeed > 0) ? max(finalRightSpeed, motor_config.min_power) : min(finalRightSpeed, -motor_config.min_power);
    }

    // Set motor speeds
    setLeftMotor(finalLeftSpeed);
    setRightMotor(finalRightSpeed);

    // Update motor status
    motors.left_speed = finalLeftSpeed;
    motors.right_speed = finalRightSpeed;

    if (DEBUG)
    {
        Serial.printf("Motors - Left: %d, Right: %d (Mapped X: %d, Y: %d, Raw X: %d, Y: %d, Button: %d)\n",
                      finalLeftSpeed, finalRightSpeed, mappedX, mappedY, x, y, button);
        Serial.printf("Normalized - X: %.2f, Y: %.2f\n", normalizedX, normalizedY);
    }
}

/**
 * Set left motor speed and direction
 */
void setLeftMotor(int speed)
{
    bool forward = speed >= 0;
    int pwm = abs(speed);

    digitalWrite(pins.in1, forward ? HIGH : LOW);
    digitalWrite(pins.in2, forward ? LOW : HIGH);
    analogWrite(pins.ena, pwm);
}

/**
 * Set right motor speed and direction
 */
void setRightMotor(int speed)
{
    bool forward = speed >= 0;
    int pwm = abs(speed);

    digitalWrite(pins.in3, forward ? HIGH : LOW);
    digitalWrite(pins.in4, forward ? LOW : HIGH);
    analogWrite(pins.enb, pwm);
}

//=============== NETWORK SETUP ===============

void setupWiFi()
{
    if (PRODUCTION_MODE)
    {
        // Production mode: AP only
        WiFi.mode(WIFI_AP);

        if (DEBUG)
        {
            Serial.println("\n=== Setting up WiFi in Production Mode (AP Only) ===");
        }
    }
    else
    {
        // Development mode: AP + Station mode
        WiFi.mode(WIFI_AP_STA);

        if (DEBUG)
        {
            Serial.println("\n=== Setting up WiFi in Development Mode (AP + Station) ===");
        }
    }

    // Setup Access Point
    WiFi.softAPConfig(config.ap_ip, config.ap_gateway, config.ap_subnet);
    WiFi.softAP(config.ap_ssid, config.ap_pass);

    if (DEBUG)
    {
        Serial.printf("Access Point SSID: %s\n", config.ap_ssid);
        Serial.printf("Access Point IP: %s\n", WiFi.softAPIP().toString().c_str());
    }

    if (!PRODUCTION_MODE)
    {
        // Only connect to home WiFi in development mode
        WiFi.begin(config.home_ssid, config.home_pass);

        // Wait for connection (with timeout)
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20)
        {
            delay(500);
            if (DEBUG)
                Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED && DEBUG)
        {
            Serial.printf("\nConnected to home network: %s\n", config.home_ssid);
            Serial.printf("Home network IP: %s\n", WiFi.localIP().toString().c_str());
        }
    }

    if (DEBUG)
    {
        Serial.println("=== WiFi Setup Complete ===\n");
    }
}

void setupServer()
{
    // Configure web server
    server.on("/", HTTP_GET, []()
              { server.send(200, "text/html", WEBPAGE); });
    server.begin();

    // Configure WebSocket
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    webSocket.enableHeartbeat(2000, 1000, 2); // Enable heartbeat to maintain connection

    // Start UDP
    udp.begin(config.udp_port);

    Serial.println("\n=== Server Configuration ===");
    Serial.printf("Web server started on port %d\n", config.web_port);
    Serial.printf("WebSocket server started on port %d\n", config.websocket_port);
    Serial.printf("UDP server listening on port %d\n", config.udp_port);
    Serial.println("============================\n");
}

//=============== MAIN FUNCTIONS ===============

// Last command values for web interface
int lastX = 0, lastY = 0, lastButton = 0;

// Add after WiFi Configuration
// Debug flags
bool debugMode = true; // Set to false to disable detailed serial output
unsigned long lastDebugOutput = 0;
const int DEBUG_INTERVAL = 1000; // Debug output every 1 second

// Connection Status
bool isWiFiConnected = false;
bool isUDPActive = false;
unsigned long lastUDPSent = 0;

/**
 * Print debug information about network and motor status
 */
void printDebugInfo()
{
    if (debugMode && (millis() - lastDebugOutput >= DEBUG_INTERVAL))
    {
        Serial.println("\n=== RC Car Status ===");

        // AP Status
        Serial.println("Access Point Status:");
        Serial.printf("- SSID: %s\n", config.ap_ssid);
        Serial.printf("- AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("- Connected Clients: %d\n", WiFi.softAPgetStationNum());

        // Home WiFi Status
        Serial.println("\nHome Network Status:");
        Serial.printf("- Connected: %s\n", WiFi.status() == WL_CONNECTED ? "Yes" : "No");
        Serial.printf("- Network: %s\n", config.home_ssid);
        Serial.printf("- IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("- Signal Strength: %d dBm\n", WiFi.RSSI());

        // Command Status
        Serial.println("\nCommand Status:");
        Serial.printf("- Last Command Age: %lums\n", millis() - status.last_command);
        Serial.printf("- Last Values - X:%d Y:%d Button:%d\n", status.last_x, status.last_y, status.last_button);

        // Motor Status
        Serial.println("\nMotor Status:");
        Serial.printf("- Left Motor: %d (%.1f%%)\n", motors.left_speed, (abs(motors.left_speed) * 100.0) / 255.0);
        Serial.printf("- Right Motor: %d (%.1f%%)\n", motors.right_speed, (abs(motors.right_speed) * 100.0) / 255.0);

        // Web Interface
        Serial.println("\nWeb Interface:");
        Serial.println("Monitor URLs:");
        Serial.printf("- AP Mode: http://%s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("- Home Network: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("- WebSocket Clients: %u\n", webSocket.connectedClients());

        Serial.println("===================\n");
        lastDebugOutput = millis();
    }
}

/**
 * Handle WebSocket events with improved error handling and variable scoping
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    // Pre-declare variables used in multiple cases
    StaticJsonDocument<200> doc;
    char buffer[200];

    switch (type)
    {
    case WStype_DISCONNECTED:
        if (DEBUG)
        {
            Serial.printf("[WebSocket] Client #%u Disconnected\n", num);
        }
        break;

    case WStype_CONNECTED:
        if (DEBUG)
        {
            Serial.printf("[WebSocket] Client #%u Connected\n", num);
        }
        // Send current configuration
        doc.clear(); // Clear document before reuse
        doc["type"] = "config";
        doc["min_power"] = motor_config.min_power;
        doc["exp_factor"] = motor_config.exp_factor;
        doc["log_factor"] = motor_config.log_factor;
        doc["boost_factor"] = motor_config.boost_factor;
        doc["deadzone"] = motor_config.deadzone;
        doc["mode"] = static_cast<int>(motor_config.mode);

        serializeJson(doc, buffer);
        webSocket.sendTXT(num, buffer);
        break;

    case WStype_TEXT:
    {
        if (length < 2)
            return; // Ignore empty messages

        // Fast path for control messages (they start with "{")
        if (payload[0] == '{')
        {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
                return;

            const char *msgType = doc["type"];
            if (!msgType)
                return;

            if (strcmp(msgType, "control") == 0)
            {
                // Direct command processing for minimal latency
                const char *cmd = doc["command"];
                if (cmd)
                {
                    processCommand((char *)cmd);
                    status.last_command = millis();
                }
            }
            else if (strcmp(msgType, "config") == 0)
            {
                handleConfigMessage(num, doc);
            }
            else if (strcmp(msgType, "reset_config") == 0)
            {
                handleResetConfig(num);
            }
        }
        break;
    }
    }
}

/**
 * Handle configuration update messages
 */
void handleConfigMessage(uint8_t clientNum, JsonDocument &doc)
{
    // Update motor configuration with all required parameters
    int new_min_power = doc["min_power"] | motor_config.min_power;
    float new_exp_factor = doc["exp_factor"] | motor_config.exp_factor;
    float new_log_factor = doc["log_factor"] | motor_config.log_factor;
    float new_boost_factor = doc["boost_factor"] | motor_config.boost_factor;
    int new_deadzone = doc["deadzone"] | motor_config.deadzone;
    int new_mode = doc["mode"] | static_cast<int>(motor_config.mode);

    // Call updateConfig with all required parameters
    motor_config.updateConfig(
        new_deadzone,
        new_min_power,
        new_exp_factor,
        new_log_factor,
        new_boost_factor,
        new_mode);

    // Send confirmation
    StaticJsonDocument<200> response;
    char buffer[200];

    response["type"] = "config_ack";
    response["status"] = "success";
    response["min_power"] = motor_config.min_power;
    response["exp_factor"] = motor_config.exp_factor;
    response["log_factor"] = motor_config.log_factor;
    response["boost_factor"] = motor_config.boost_factor;
    response["deadzone"] = motor_config.deadzone;
    response["mode"] = static_cast<int>(motor_config.mode);

    serializeJson(response, buffer);
    webSocket.sendTXT(clientNum, buffer);

    if (DEBUG)
    {
        Serial.println("[WebSocket] Configuration updated successfully");
    }
}

/**
 * Handle reset configuration messages
 */
void handleResetConfig(uint8_t clientNum)
{
    motor_config.resetToDefaults();

    // Send confirmation
    StaticJsonDocument<200> response;
    char buffer[200];

    response["type"] = "config_ack";
    response["status"] = "reset_success";
    response["min_power"] = motor_config.min_power;
    response["exp_factor"] = motor_config.exp_factor;
    response["log_factor"] = motor_config.log_factor;
    response["boost_factor"] = motor_config.boost_factor;
    response["deadzone"] = motor_config.deadzone;
    response["mode"] = static_cast<int>(motor_config.mode);

    serializeJson(response, buffer);
    webSocket.sendTXT(clientNum, buffer);

    if (DEBUG)
    {
        Serial.println("[WebSocket] Configuration reset to defaults");
    }
}

/**
 * Handle control messages
 */
void handleControlMessage(uint8_t clientNum, JsonDocument &doc)
{
    const char *command = doc["command"];
    if (!command)
    {
        if (DEBUG)
        {
            Serial.println("[WebSocket] Missing command in control message");
        }
        return;
    }

    processCommand((char *)command);
    status.last_command = millis();

    // Send acknowledgment
    StaticJsonDocument<200> response;
    char buffer[200];

    response["type"] = "control_ack";
    response["left_speed"] = motors.left_speed;
    response["right_speed"] = motors.right_speed;
    response["command"] = command;

    serializeJson(response, buffer);
    webSocket.sendTXT(clientNum, buffer);
}

/**
 * Send status update to web clients
 */
void updateWebClients()
{
    if (millis() - status.last_web_update >= timing.web_update_interval)
    {
        StaticJsonDocument<200> doc;
        doc["type"] = "status_update";
        doc["left_speed"] = motors.left_speed;
        doc["right_speed"] = motors.right_speed;
        doc["last_x"] = status.last_x;
        doc["last_y"] = status.last_y;
        doc["last_button"] = status.last_button;
        doc["command_age"] = millis() - status.last_command;
        doc["raw_input"] = true; // Flag to indicate this is from physical controller

        char buffer[200];
        serializeJson(doc, buffer);
        webSocket.broadcastTXT(buffer);

        status.last_web_update = millis();
    }
}

/**
 * Process received UDP command
 * Format: "X,Y,B" where X,Y are joystick values (-100 to 100) and B is button state
 */
void processCommand(char *command)
{
    int x = 0, y = 0, b = 0;

    // Fast string parsing without sscanf
    char *ptr = command;
    x = atoi(ptr);
    while (*ptr && *ptr != ',')
        ptr++;
    if (*ptr == ',')
    {
        ptr++;
        y = atoi(ptr);
        while (*ptr && *ptr != ',')
            ptr++;
        if (*ptr == ',')
        {
            ptr++;
            b = atoi(ptr);
        }
    }

    // Store values
    status.last_x = x;
    status.last_y = y;
    status.last_button = b;

    // Update last command time
    status.last_command = millis();

    // Process through our motor control system
    setMotorSpeeds(x, y, b);
}

/**
 * Print initial network information
 */
void printNetworkInfo()
{
    Serial.println("\n=== Network Information ===");
    Serial.printf("Mode: %s\n", PRODUCTION_MODE ? "Production (AP Only)" : "Development (AP + Station)");
    Serial.println("Access car monitor at:");
    Serial.printf("AP Mode: http://%s\n", WiFi.softAPIP().toString().c_str());

    if (!PRODUCTION_MODE && WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("Home Network: http://%s\n", WiFi.localIP().toString().c_str());
    }

    Serial.println("=============================\n");
}

/**
 * Handle incoming UDP commands
 */
void handleUDPCommands()
{
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        char packet[config.udp_packet_size];
        int len = udp.read(packet, config.udp_packet_size);
        if (len > 0)
        {
            packet[len] = '\0';

            if (DEBUG)
            {
                Serial.printf("[UDP] Command received: %s\n", packet);
            }

            processCommand(packet);
            status.udp_active = true;
        }
    }
}

/**
 * Setup LED pins and initial states
 */
void setupLEDs()
{
    pinMode(pins.led_ready, OUTPUT);
    pinMode(pins.led_connection, OUTPUT);
    digitalWrite(pins.led_ready, LOW);
    digitalWrite(pins.led_connection, HIGH);
    status.startup_time = millis();
}

/**
 * Update LED states based on system status
 */
void updateLEDs()
{
    // Update ready LED
    if (!status.is_ready && (millis() - status.startup_time >= timing.ready_delay))
    {
        status.is_ready = true;
        digitalWrite(pins.led_ready, HIGH);
    }

    // Update connection LED
    unsigned long time_since_last_command = millis() - status.last_command;

    if (time_since_last_command < timing.connection_timeout)
    {
        // Blink LED when receiving commands
        if (millis() - status.last_led_blink >= timing.connection_blink)
        {
            status.led_connection_state = !status.led_connection_state;
            digitalWrite(pins.led_connection, status.led_connection_state);
            status.last_led_blink = millis();
        }
    }
    else
    {
        // Turn off LED if no recent commands
        digitalWrite(pins.led_connection, LOW);
        status.led_connection_state = false;
    }
}
