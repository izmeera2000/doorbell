#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

// WiFi credentials
const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

// WebSocket server
AsyncWebServer server(81);
AsyncWebSocket ws("/ws");

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;

// Streaming buffer
uint8_t* audioBuffer = nullptr;
size_t bufferSize = 0;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure I2S for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.5);            // Adjust volume
  out->SetPinout(26, 25, 27);   // BCK=GPIO26, WS=GPIO25, DATA=GPIO27

  // WebSocket event handler
  ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;

      // Handle binary data for real-time audio streaming
      if (info->opcode == WS_BINARY) {
        Serial.printf("Received binary chunk of size %d\n", len);

        // Allocate a buffer for incoming audio data
        if (audioBuffer == nullptr) {
          bufferSize = len;
          audioBuffer = (uint8_t*)malloc(bufferSize);
          if (!audioBuffer) {
            Serial.println("Failed to allocate audio buffer!");
            client->text("Error: Unable to allocate buffer");
            return;
          }
        }

        // Copy the received data into the buffer
        memcpy(audioBuffer, data, len);

        // Play the audio data directly
        if (out->ConsumeSample((const int16_t*)audioBuffer, len / sizeof(int16_t))) {
          Serial.println("Audio data played");
        } else {
          Serial.println("Failed to play audio data");
        }
      }
    }
  });

  // Add WebSocket endpoint to server
  server.addHandler(&ws);

  // Start server
  server.begin();
}

void loop() {
  // Nothing specific needed in the loop; WebSocket handles data asynchronously
}
