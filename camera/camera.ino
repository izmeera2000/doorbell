#include "esp_camera.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>
#include <LittleFS.h>

// Camera configuration and setup code (same as your original camera setup)

const char *ssid = "iPhone";
const char *password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;
AudioFileSourceLittleFS* fileSource = nullptr;

// Async server
AsyncWebServer server(81);

// Global file handle for writing
File audioFile;

void startCameraServer();  // Declaring the camera server start function

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Setup camera
  camera_config_t config;
  // (Your camera configuration code here, as in your example...)

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // WiFi setup
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Start camera server
  startCameraServer();

  // Initialize LittleFS for audio
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Configure I2S output for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true);
  out->SetGain(0.1);
  out->SetPinout(0, 0, 25);  // Adjust GPIO for audio output

  // Set up HTTP POST endpoint for receiving audio
  server.on("/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      Serial.printf("Chunk received: Index=%d, Len=%d, Total=%d\n", index, len, total);

      if (index == 0) {
        Serial.println("Receiving audio data...");
        audioFile = LittleFS.open("/audio.wav", FILE_WRITE);
        if (!audioFile) {
          Serial.println("Failed to open file for writing");
          request->send(500, "text/plain", "Failed to open file for writing");
          return;
        }
      }

      // Write received data to the file
      if (audioFile) {
        audioFile.write(data, len);
      }

      if (index + len == total) {
        Serial.println("Audio data received, finalizing file...");
        if (audioFile) {
          audioFile.close();
        }

        // Stop previous playback if running
        if (wav && wav->isRunning()) {
          wav->stop();
          delete wav;
          wav = nullptr;
        }
        if (fileSource) {
          delete fileSource;
          fileSource = nullptr;
        }

        // Initialize WAV playback
        fileSource = new AudioFileSourceLittleFS("/audio.wav");
        wav = new AudioGeneratorWAV();
        if (wav->begin(fileSource, out)) {
          Serial.println("Audio playback started");
          request->send(200, "text/plain", "Audio received and playing");
        } else {
          Serial.println("Failed to start WAV decoder!");
          request->send(500, "text/plain", "Failed to start WAV decoder");
        }
      }
    });

  // Start the server
  server.begin();

  // Camera ready message
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
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
  // Add any additional tasks if necessary (camera streaming, etc.)
  delay(100);  // Placeholder to avoid watchdog timer reset
}
