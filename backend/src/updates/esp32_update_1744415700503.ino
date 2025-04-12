#include <WiFi.h>
#include <WebServer.h>

// Replace these with your Wi-Fi credentials
const char* ssid = "dlink-C2DB";
const char* password = "7gyjw7et";

#define RELAY_PIN 5  // Update with the pin controlling your relay

// Create a web server on port 80
WebServer server(80);

void openDoor() {
  digitalWrite(RELAY_PIN, HIGH);  // Activate relay—adjust logic if your relay is active LOW
  delay(2000);                    // Hold for 2 seconds
  digitalWrite(RELAY_PIN, LOW);   // Deactivate relay
}

// Handler for the "/open" endpoint
void handleOpen() {
  server.send(200, "text/plain", "Door Opened");
  openDoor();
}

void setup() {
  Serial.begin(115200);
  
  // Set up the relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Ensure relay starts off

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up the web server endpoint
  server.on("/open", handleOpen);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
