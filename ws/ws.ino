#include <WiFi.h>
#include <WebSocketsServer.h>

WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(115200);

  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    // Handle incoming text data (audio stream)
    // You can either save it or directly forward it to an audio output
  }
  if (type == WStype_BIN) {
    // Handle incoming binary data (audio data from Flutter)
    Serial.printf("Received audio data: %u bytes\n", length);
    // Send the audio data back to the client (Flutter app)
    webSocket.sendBIN(num, payload, length);
  }
}
