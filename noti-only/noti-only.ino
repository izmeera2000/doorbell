#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Pusher API credentials
const String app_id = "1897337";
const String key = "3ef10ab69edd1c712eeb";
const String secret = "d21e52ac4ffabdc37745";
const String cluster = "ap1";

// Pusher API URL
const String url = "http://api.pusherapp.com/apps/" + app_id + "/events";

// Set the channel and event to trigger
const String channel = "doorbell";
const String event = "bell";

void sendPusherNotification() {
  HTTPClient http;

  // Build the JSON data to send
  String data = "{\"name\":\"" + event + "\",\"channel\":\"" + channel + "\",\"data\":\"{\\\"message\\\":\\\"Doorbell pressed!\\\"}\"}";

  // Set the Authorization header
  String authHeader = "Bearer " + secret;

  // Initialize HTTP request
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authHeader);

  // Send the POST request to Pusher
  int httpResponseCode = http.POST(data);

  // Check for successful response
  if (httpResponseCode > 0) {
    Serial.println("Notification sent successfully");
  } else {
    Serial.print("Error sending notification. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }

  // End the HTTP connection
  http.end();
}

void setup() {
  // Start Serial Monitor for debugging
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Successfully connected to WiFi
  Serial.println();
  Serial.println("Connected to WiFi");

  // Send the Pusher notification
  sendPusherNotification();
}

void loop() {
  // The loop does nothing in this case, just sending the notification once in setup
  // Add other logic here if needed
}
