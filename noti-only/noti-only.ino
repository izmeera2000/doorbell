#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* pusherKey = "3ef10ab69edd1c712eeb";
const char* pusherCluster = "ap1";
const char* pusherSecret = "d21e52ac4ffabdc37745";
const char* channelName = "doorbell";
const char* eventName = "ring";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  // Simulate a doorbell ring
  if (digitalRead(4) == HIGH) {  // Replace with your button pin
    sendNotification();
    delay(5000);  // Debounce or avoid rapid calls
  }
}

void sendNotification() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://" + String(pusherCluster) + ".pusher.com/apps/YOUR_APP_ID/events";

    String payload = "{"
                     "\"name\": \"" + String(eventName) + "\","
                     "\"channels\": [\"" + String(channelName) + "\"],"
                     "\"data\": \"{\\\"message\\\": \\\"Doorbell Pressed!\\\"}\""
                     "}";

    http.begin(url.c_str());
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(pusherSecret));
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.printf("Notification sent! Response code: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error sending notification. HTTP error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
