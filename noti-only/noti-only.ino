#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <string.h>
#include <mbedtls/md.h> // For HMAC-SHA256

// Wi-Fi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Pusher API credentials
const String app_id = "1897337";
const String key = "3ef10ab69edd1c712eeb";
const String secret = "d21e52ac4ffabdc37745";
const String cluster = "ap1";

// Pusher API URL
const String url = "http://api-" + cluster + ".pusher.com/apps/" + app_id + "/events";

// Set the channel and event to trigger
const String channel = "doorbell";
const String event = "bell";

// Function to create the authentication signature
String createAuthSignature(String data) {
  unsigned long timestamp = millis() / 1000;  // Use timestamp in seconds
  String stringToHash = "/apps/" + app_id + "/events?body=" + data + "&auth_key=" + key + "&auth_timestamp=" + String(timestamp) + "&auth_version=1.0";
  
  // HMAC-SHA256 hashing using mbedtls
  unsigned char hmacHash[32]; // HMAC-SHA256 produces a 32-byte hash
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret.c_str(), secret.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)stringToHash.c_str(), stringToHash.length());
  mbedtls_md_hmac_finish(&ctx, hmacHash);
  mbedtls_md_free(&ctx);

  // Convert the hash result to a hex string for the signature
  String signature = "";
  for (int i = 0; i < 32; i++) {
    signature += String(hmacHash[i], HEX);
  }
  return signature;
}

// Function to send Pusher notification
void sendPusherNotification() {
  HTTPClient http;

  // Build the JSON data to send
  String data = "{\"name\":\"" + event + "\",\"channel\":\"" + channel + "\",\"data\":\"{\\\"message\\\":\\\"Doorbell pressed!\\\"}\"}";

  // Create the authentication signature
  String signature = createAuthSignature(data);

  // Construct the full URL with query parameters
  String fullUrl = url + "?body=" + data + "&auth_key=" + key + "&auth_timestamp=" + String(millis() / 1000) + "&auth_version=1.0&auth_signature=" + signature;

  // Initialize HTTP request
  http.begin(fullUrl);
  http.addHeader("Content-Type", "application/json");

  // Send the POST request to Pusher
  int httpResponseCode = http.POST(data);

  // Check for successful response
  if (httpResponseCode > 0) {
    Serial.println("Notification sent successfully");

    // Get and print the response body
    String response = http.getString();
    Serial.println("Response: " + response);
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
