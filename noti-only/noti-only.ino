#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi Credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Pusher configuration
const char *app_id = "1897337";
const char *key = "3ef10ab69edd1c712eeb";
const char *secret = "d21e52ac4ffabdc37745";
const char *cluster = "ap1";
const char *channel = "doorbell";
const char *event = "bell";

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  // Send Pusher notification
  if (sendPusherNotification()) {
    Serial.println("Notification sent successfully");
  } else {
    Serial.println("Failed to send notification");
  }
}

void loop() {
  // Add your main loop logic if needed
}

bool sendPusherNotification() {
  HTTPClient http;

  // Construct the Pusher REST API URL
  String url = "http://" + String(cluster) + "-api.pusher.com/apps/" + String(app_id) + "/events";

  // Generate the data payload for the notification
  String payload = "{\"name\":\"" + String(event) + "\",\"channels\":[\"" + String(channel) + "\"],\"data\":\"{\\\"message\\\":\\\"Doorbell pressed!\\\"}\"}";

  // Calculate the authentication signature
  String body_md5 = md5(payload);
  String timestamp = String(millis() / 1000);
  String string_to_sign = "POST\n/apps/" + String(app_id) + "/events\nauth_key=" + String(key) +
                          "&auth_timestamp=" + timestamp + "&auth_version=1.0&body_md5=" + body_md5;
  String auth_signature = hmac_sha256(secret, string_to_sign);

  // Construct the full query parameters
  String params = "auth_key=" + String(key) + "&auth_timestamp=" + timestamp + "&auth_version=1.0&body_md5=" + body_md5 + "&auth_signature=" + auth_signature;

  // Open HTTP connection
  http.begin(url + "?" + params);
  http.addHeader("Content-Type", "application/json");

  // Send the request
  int httpResponseCode = http.POST(payload);

  // Check the response
  if (httpResponseCode == 200) {
    Serial.println("Notification sent successfully");
    http.end();
    return true;
  } else {
    Serial.printf("HTTP Response Code: %d\n", httpResponseCode);
    Serial.println(http.getString());
    http.end();
    return false;
  }
}

// Helper function to calculate MD5 hash
String md5(String payload) {
  char md5_hash[33];
  MD5Builder md5;
  md5.begin();
  md5.add(payload);
  md5.calculate();
  md5.toString(md5_hash, sizeof(md5_hash));
  return String(md5_hash);
}

// Helper function to calculate HMAC-SHA256 signature
String hmac_sha256(const String &key, const String &message) {
  uint8_t key_buffer[32];
  size_t key_length = key.length();
  key.getBytes(key_buffer, key_length);

  uint8_t hash_output[32];
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, info, 1);
  mbedtls_md_hmac_starts(&ctx, key_buffer, key_length);
  mbedtls_md_hmac_update(&ctx, (uint8_t *)message.c_str(), message.length());
  mbedtls_md_hmac_finish(&ctx, hash_output);
  mbedtls_md_free(&ctx);

  String result;
  for (int i = 0; i < 32; i++) {
    char hex[3];
    sprintf(hex, "%02x", hash_output[i]);
    result += hex;
  }
  return result;
}
