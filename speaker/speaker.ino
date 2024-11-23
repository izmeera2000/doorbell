#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSPIFFS.h>
#include <SPIFFS.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;
AudioFileSourceSPIFFS* fileSource = nullptr;

// Async server
AsyncWebServer server(82);

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

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS mount failed!");
    return;
  }

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
                File file = SPIFFS.open("/audio.wav", FILE_WRITE);
                if (!file) {
                  Serial.println("Failed to open file for writing");
                  request->send(500, "text/plain", "Failed to open file for writing");
                  return;
                }
                file.close();
              }

              // Append received data to the file
              File file = SPIFFS.open("/audio.wav", FILE_APPEND);
              if (file) {
                file.write(data, len);
                file.close();
              }

              if (index + len == total) {
                Serial.println("Audio data received, starting playback...");

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
                fileSource = new AudioFileSourceSPIFFS("/audio.wav");
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
