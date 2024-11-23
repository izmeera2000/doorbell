#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV *wav = nullptr;
AudioOutputI2S *out = nullptr;

// Async server
AsyncWebServer server(81);

// Temporary buffer for received audio
#define AUDIO_BUFFER_SIZE 4096
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
size_t audioBufferIndex = 0;

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
  }
  Serial.println("WiFi connected!");

  // Configure I2S output for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.2);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // Use GPIO25 for DAC (BCK and WS set to 0)

  // Set up HTTP POST endpoint for receiving audio
  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              if (index == 0) {
                Serial.println("Receiving audio data...");
                audioBufferIndex = 0;
              }

              // Append received data to buffer
              if (audioBufferIndex + len <= AUDIO_BUFFER_SIZE) {
                memcpy(audioBuffer + audioBufferIndex, data, len);
                audioBufferIndex += len;
              }

              // When all data is received
              if (index + len == total) {
                Serial.println("Audio data received, starting playback...");

                // Stop previous playback if running
                if (wav && wav->isRunning()) {
                  wav->stop();
                }

                // Initialize WAV playback
                wav = new AudioGeneratorWAV();
                wav->begin(new AudioFileSourceBuffer(audioBuffer, audioBufferIndex), out);

                // Respond to the client
                request->send(200, "text/plain", "Audio received and playing");
              }
            });

  // Start the server
  server.begin();
}

void loop() {
  // Process the audio stream (if active)
  if (wav != nullptr && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = nullptr;
      Serial.println("Audio playback finished.");
    }
  }
}
