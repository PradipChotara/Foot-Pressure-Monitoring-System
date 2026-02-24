
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

// --- Configuration ---
const char* ssid = "Nord";
const char* password = "123123123";

// --- WebSocket Server Configuration ---
const int   websockets_server_port = 5000;  // Flask-SocketIO port

// --- Sampling Frequency Configuration ---
// Can be changed at runtime by commands from the server
volatile unsigned long SAMPLING_INTERVAL_MS = 25;

// --- Static IP Configuration (for the ESP32) ---

const char* DEVICE_ID = "left";
IPAddress staticIP(10, 215, 59, 101);  // ESP32 Left's IP (Assign one here for left module)
// const char* DEVICE_ID = "right";
// IPAddress staticIP(10, 63, 196, 102);  // ESP32 Right's IP (Assign one here for right module)

const char* websockets_server_host = "10.215.59.10";  // IPv4 Address (Backend Server IP)

IPAddress gateway(10, 215, 59, 230);   // Default Gateway (router's IP)
IPAddress subnet(255, 255, 255, 0);    // Subnet Mask





// --- Global variables ---
WebSocketsClient webSocket;
unsigned long previousMillis = 0;

// --- Pin Definitions for FSR Sensors ---
#define FSR1_PIN 34
#define FSR2_PIN 35
#define FSR3_PIN 32
#define FSR4_PIN 33

// --- Function Prototypes ---
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
float readResistance(int pin);
float resistanceToWeight(float R);
float kgToNewton(float weightKg);
void readAndSendData();

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Disconnected!");
      break;

    case WStype_CONNECTED:
      Serial.printf("[WebSocket] Connected to: %s\n", payload);
      webSocket.sendTXT("40"); // Socket.IO CONNECT packet
      break;

    case WStype_TEXT: { // Use braces to create a local scope for variables
      // First, handle the Engine.IO heartbeat (ping/pong)
      if (length == 1 && payload[0] == '2') {
        webSocket.sendTXT("3"); // Send pong
        return; // Exit the function
      }

      // NEW: Check for a Socket.IO event packet (it starts with "42")
      if (length > 2 && payload[0] == '4' && payload[1] == '2') {
        // We received a command from the server, let's parse it
        StaticJsonDocument<128> doc;
        // Start parsing the JSON from the 3rd character (index 2)
        DeserializationError error = deserializeJson(doc, &payload[2]);

        if (error) {
          Serial.print(F("JSON parsing failed: "));
          Serial.println(error.c_str());
          return;
        }

        // The expected format is ["event_name", { "key": "value" }]
        const char* eventName = doc[0];
        
        // Check if the event is the one we care about
        if (strcmp(eventName, "update_rate") == 0) {
          int newRate = doc[1]["rate"]; // Get the new rate value
          if (newRate > 0 && newRate <= 40) {
             SAMPLING_INTERVAL_MS = 1000 / newRate;
             Serial.printf("[COMMAND] New rate received: %d Hz. Interval updated to %lu ms.\n", newRate, SAMPLING_INTERVAL_MS);
          }
        }
      }
      break;
    }

    case WStype_ERROR:
      Serial.printf("[WebSocket] Error: %.*s\n", (int)length, (const char*)payload);
      break;
      
    default:
      break;
  }
}

float readResistance(int pin) {
  int adc = analogRead(pin);
  float voltage = (adc / 4095.0) * 3.3;
  if (voltage < 0.05) return 0;
  return (3.3 - voltage) * 10000.0 / voltage;
}

float resistanceToWeight(float R) {
  if (R <= 0) return 0;
  return 3454.26 * pow(R, -1.01);
}

float kgToNewton(float weightKg) {
  return weightKg * 9.81;
}

String getTimestamp() {
  unsigned long ms = millis();

  unsigned long total_seconds = ms / 1000;
  unsigned int milliseconds = ms % 1000;
  unsigned int seconds = total_seconds % 60;
  unsigned int minutes = (total_seconds / 60) % 60;
  unsigned int hours = (total_seconds / 3600) % 24;

  char timestamp[13]; // HH:MM:SS:mmm + null terminator
  snprintf(timestamp, sizeof(timestamp), "%02u:%02u:%02u:%03u", hours, minutes, seconds, milliseconds);
  return String(timestamp);
}


void readAndSendData() {
  if (!webSocket.isConnected()) return;

  StaticJsonDocument<256> doc;
  doc["id"] = DEVICE_ID;
  doc["timestamp"] = getTimestamp();
  doc["force1_N"] = kgToNewton(resistanceToWeight(readResistance(FSR1_PIN)));
  doc["force2_N"] = kgToNewton(resistanceToWeight(readResistance(FSR2_PIN)));
  doc["force3_N"] = kgToNewton(resistanceToWeight(readResistance(FSR3_PIN)));
  doc["force4_N"] = kgToNewton(resistanceToWeight(readResistance(FSR4_PIN)));
  doc["totalForce_N"] =
    doc["force1_N"].as<float>() +
    doc["force2_N"].as<float>() +
    doc["force3_N"].as<float>() +
    doc["force4_N"].as<float>();

  String jsonOutput;
  serializeJson(doc, jsonOutput);

  // Socket.IO v4 event frame:
  // "42" = "4" (Engine.IO msg) + "2" (Socket.IO EVENT)
  // Event payload: ["sensor_data", {...}]
  String sio = "42[\"sensor_data\"," + jsonOutput + "]";
  webSocket.sendTXT(sio);

  Serial.printf("Sent SIO: %s\n", sio.c_str());
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Connect directly to the Engine.IO v4 WebSocket transport for Socket.IO
  // No extra headers required; the library performs the RFC6455 handshake
  webSocket.begin(websockets_server_host, websockets_server_port,
                  "/socket.io/?EIO=4&transport=websocket");

  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= SAMPLING_INTERVAL_MS) {
    previousMillis = currentMillis;
    readAndSendData();
  }
}
