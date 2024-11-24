#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceBuffer.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";
const char *password = "Alamak323";

// DAC Pin for Speaker
#define DAC_PIN 25

// Web Server
AsyncWebServer server(82);

// WAV Audio Processing
AudioGeneratorWAV *wav = nullptr;
AudioFileSourceBuffer *fileSource = nullptr;

// Buffer for storing the incoming audio stream
#define AUDIO_BUFFER_SIZE 8192
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
size_t bufferIndex = 0;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to initialize LittleFS");
    return;
  }

  // Endpoint for receiving and playing WAV audio
  server.on("/stream", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Streaming audio...");
  }, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Append incoming data to the buffer
    if ((bufferIndex + len) <= AUDIO_BUFFER_SIZE) {
      memcpy(audioBuffer + bufferIndex, data, len);
      bufferIndex += len;
    }

    // If all data received, start playback
    if ((index + len) == total) {
      Serial.println("Received full audio stream");
      fileSource = new AudioFileSourceBuffer(audioBuffer, bufferIndex);
      wav = new AudioGeneratorWAV();
      if (wav->begin(fileSource, nullptr)) {
        Serial.println("Playing audio...");
        while (wav->isRunning()) {
          if (wav->loop()) {
            int16_t sample = wav->read();  // Read decoded WAV sample
            int dacValue = (sample + 32768) >> 8;  // Convert 16-bit signed to 8-bit unsigned
            dacWrite(DAC_PIN, dacValue);  // Output to DAC
          } else {
            wav->stop();
          }
        }
      } else {
        Serial.println("Failed to play audio");
      }

      // Clean up
      delete wav;
      delete fileSource;
      wav = nullptr;
      fileSource = nullptr;
      bufferIndex = 0;
    }
  });

  server.begin();
}

vo
