#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>
#include <LittleFS.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";


// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;
AudioFileSourceLittleFS* fileSource = nullptr;

// Async web server
AsyncWebServer server(81);

// Global file handle for writing
File audioFile;

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
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Configure I2S output for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.5);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(14, 25, 27);   // BCK=GPIO26, WS=GPIO25, DATA=GPIO27 (modify as needed)

  // Define HTTP POST endpoint for receiving audio
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

  // Start the web server
  server.begin();
  Serial.println("Server started");
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
