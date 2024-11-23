#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";


// Pusher Channels API URL
const String PUSHER_API_URL = "https://api.pusherapp.com/apps/1897337/events";
const String API_KEY = "3ef10ab69edd1c712eeb";  // Your Pusher API key
const String CLUSTER = "ap1";  // Your Pusher cluster

// Define channel and event name
const String CHANNEL_NAME = "doorbell";
const String EVENT_NAME = "ring";

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");
  
  // Send event to Pusher
  sendPusherEvent();
}

void loop() {
  // Nothing to do here
}

void sendPusherEvent() {
  HTTPClient http;
  
  // Set up the HTTP request
  http.begin(PUSHER_API_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + API_KEY);  // Authorization header
  
  // Build the JSON payload
  String payload = String("{")
    + "\"name\": \"" + EVENT_NAME + "\","
    + "\"channel\": \"" + CHANNEL_NAME + "\","
    + "\"data\": {\"message\": \"Hello from ESP32!\"}"
    + "}";

  // Send the HTTP POST request
  int httpResponseCode = http.POST(payload);
  
  if (httpResponseCode > 0) {
    Serial.println("Event sent to Pusher successfully!");
    Serial.println("HTTP Response code: " + String(httpResponseCode));
  } else {
    Serial.println("Error sending event to Pusher: " + String(httpResponseCode));
  }
  
  http.end();  // Free resources
}
