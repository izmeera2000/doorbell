#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Pusher API URL and App ID
const String PUSHER_API_URL = "https://api.pusherapp.com/apps/1897337/events";  // Replace with your App ID
const String API_KEY = "d21e52ac4ffabdc37745";  // Replace with your Pusher Master Key

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
  http.begin(PUSHER_API_URL);  // The full Pusher API URL
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + API_KEY);  // Use Master Key for authorization
  
  // Build the JSON payload
  String payload = String("{")
    + "\"name\": \"" + EVENT_NAME + "\","
    + "\"channel\": \"" + CHANNEL_NAME + "\","
    + "\"data\": {\"message\": \"hello world\"}"
    + "}";

  // Send the HTTP POST request
  int httpResponseCode = http.POST(payload);
  
  // Check if request was successful
  if (httpResponseCode > 0) {
    // Read the response payload (string format)
    String response = http.getString();
    Serial.println("Event sent to Pusher successfully!");
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);  // The response body
  } else {
    // If the request fails, print the error code
    Serial.println("Error sending event to Pusher: " + String(httpResponseCode));
    Serial.println("Response: " + http.errorToString(httpResponseCode));
  }
  
  http.end();  // Free resources
}
