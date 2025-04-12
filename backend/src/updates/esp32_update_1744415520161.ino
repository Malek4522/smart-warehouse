#include <Arduino.h>

// Sensor Configuration
const uint8_t sensorCount = 6;
const uint8_t sensorPins[] = {A0, A1, A2, A3, A4, A5};
uint16_t minValues[sensorCount];
uint16_t maxValues[sensorCount];

// PID Variables
float Kp = 0.8;
float Ki = 0.002;
float Kd = 2.0;
float lastError = 0;
float integral = 0;
unsigned long lastTime = 0;

// Motor Pins
const uint8_t leftMotorPWM = 5;//to be deleted
const uint8_t rightMotorPWM = 6;//to be deleted
const uint8_t leftMotorDir = 7;//to be deleted
const uint8_t rightMotorDir = 8;//to be deleted

// Speed Settings
const int minSpeed = 60;
const int maxSpeed = 150;

// Failsafe Variables
const uint16_t lostThreshold = 200; // If total sensor readings drop below this, assume the line is lost
const unsigned long failsafeTimeout = 1000; // Time (ms) before stopping completely
unsigned long lastDetectedTime = 0;

void setup() {
    Serial.begin(9600);

    pinMode(leftMotorPWM, OUTPUT);
    pinMode(rightMotorPWM, OUTPUT);
    pinMode(leftMotorDir, OUTPUT);
    pinMode(rightMotorDir, OUTPUT);

    for (uint8_t i = 0; i < sensorCount; i++) {
        minValues[i] = 1023;
        maxValues[i] = 0;
    }

    Serial.println("Calibrating... Move the robot over black and white areas!");
    for (uint8_t i = 0; i < 100; i++) {
        calibrateSensors();
        delay(10);
    }
    Serial.println("Calibration complete!");
    lastTime = millis();
    lastDetectedTime = millis(); // Start failsafe timer
}

void loop() {
    float position = getPosition();
    float error = position - (sensorCount - 1) * 500 / 2;

    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    if (deltaTime <= 0) return;

    integral += error * deltaTime;
    float derivative = (error - lastError) / deltaTime;
    float correction = Kp * error + Ki * integral + Kd * derivative;
    lastError = error;

    int baseSpeed = calculateAdaptiveSpeed();
    int leftSpeed = baseSpeed + correction;
    int rightSpeed = baseSpeed - correction;

    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);

    // 🔥 Failsafe Check 🔥
    if (isLineLost()) {
        if (millis() - lastDetectedTime > failsafeTimeout) {
            stopMotors();
            Serial.println("❌ Line lost! Stopping robot.");
            return;
        }
    } else {
        lastDetectedTime = millis(); // Reset failsafe timer
    }

    moveMotors(leftSpeed, rightSpeed);
}

// 🛠 Custom Calibration Function
void calibrateSensors() {
    for (uint8_t i = 0; i < sensorCount; i++) {
        uint16_t value = analogRead(sensorPins[i]);

        if (value < minValues[i]) minValues[i] = value;
        if (value > maxValues[i]) maxValues[i] = value;
    }
}

// 📍 Get Normalized Position
float getPosition() {
    uint32_t weightedSum = 0;
    uint16_t sum = 0;

    for (uint8_t i = 0; i < sensorCount; i++) {
        uint16_t rawValue = analogRead(sensorPins[i]);
        uint16_t normalized = map(rawValue, minValues[i], maxValues[i], 0, 1000);
        normalized = constrain(normalized, 0, 1000);

        weightedSum += normalized * (i * 1000);
        sum += normalized;
    }

    return (sum > 0) ? (weightedSum / sum) : (sensorCount - 1) * 500 / 2;
}

// ⚡ Adaptive Speed Control
int calculateAdaptiveSpeed() {
    uint16_t sum = 0;

    for (uint8_t i = 0; i < sensorCount; i++) {
        uint16_t rawValue = analogRead(sensorPins[i]);
        uint16_t normalized = map(rawValue, minValues[i], maxValues[i], 0, 1000);
        normalized = constrain(normalized, 0, 1000);
        sum += normalized;
    }

    return map(sum, 0, sensorCount * 1000, minSpeed, maxSpeed);
}

// ⚠️ Check if Line is Lost
bool isLineLost() {
    uint16_t total = 0;
    for (uint8_t i = 0; i < sensorCount; i++) {
        uint16_t rawValue = analogRead(sensorPins[i]);
        uint16_t normalized = map(rawValue, minValues[i], maxValues[i], 0, 1000);
        total += normalized;
    }
    return total < lostThreshold;
}

// 🛑 Stop Motors
void stopMotors() {
    analogWrite(leftMotorPWM, 0);
    analogWrite(rightMotorPWM, 0);
}

// ⚡ Motor Control
void moveMotors(int leftSpeed, int rightSpeed) {
    digitalWrite(leftMotorDir, HIGH);
    digitalWrite(rightMotorDir, HIGH);

    analogWrite(leftMotorPWM, leftSpeed);
    analogWrite(rightMotorPWM, rightSpeed);
}
