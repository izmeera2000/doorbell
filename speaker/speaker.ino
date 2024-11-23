#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceBuffer.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;

// Buffer for audio data
#define AUDIO_BUFFER_SIZE 8192
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
size_t audioBufferIndex = 0;

AsyncWebServer server(81);

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
  }
  Serial.println("WiFi connected!");

  // Configure I2S output
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.2);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // DAC on GPIO25 (BCK and WS are unused)

  // Set up HTTP POST endpoint
  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
            [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
              Serial.printf("Chunk received: Index=%d, Len=%d, Total=%d\n", index, len, total);

              // Store received data in buffer
              if (audioBufferIndex + len <= AUDIO_BUFFER_SIZE) {
                memcpy(audioBuffer + audioBufferIndex, data, len);
                audioBufferIndex += len;
              } else {
                Serial.println("Audio buffer overflow!");
                request->send(500, "text/plain", "Buffer overflow");
                return;
              }

              // If all data received, start playback
              if (index + len == total) {
                Serial.println("All audio data received!");

                if (wav && wav->isRunning()) {
                  wav->stop();
                  delete wav;
                }

                wav = new AudioGeneratorWAV();
                auto* audioSource = new AudioFileSourceBuffer(audioBuffer, audioBufferIndex);

                if (wav->begin(audioSource, out)) {
                  Serial.println("Audio playback started");
                  request->send(200, "text/plain", "Audio uploaded and playing");
                } else {
                  Serial.println("Failed to start playback");
                  request->send(500, "text/plain", "Failed to start playback");
                  delete audioSource;
                }

                // Reset buffer index for next upload
                audioBufferIndex = 0;
              }
            });

  server.begin();
}

void loop() {
  // Process audio playback
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = nullptr;
      Serial.println("Audio playback finished.");
    }
  }
}
