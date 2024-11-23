#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceLittleFS.h>
#include <LittleFS.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioFileSourceLittleFS* fileSource = nullptr;

// Async server
AsyncWebServer server(82);

// Global file handle for writing
File audioFile;

// DAC pin for audio output (GPIO25 for example)
const int audioPin = 25; // Change if you use a different DAC pin (GPIO26)

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

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Set DAC output pin (assuming GPIO25 or GPIO26)
  dacWrite(audioPin, 0); // Initialize the DAC output

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
                if (wav->begin(fileSource, nullptr)) {
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
    if (wav->loop()) {
      // Read an 8-bit sample from the WAV file
      uint8_t sample = wav->ReadU8(); // Get the next audio sample (8-bit)

      // Map the sample value to the range of DAC output (0-255)
      // The DAC input range is 0-255, so we need to map the sample from a byte (0-255)
      // You may need to adjust the mapping for higher precision if using 16-bit samples
      int dacValue = sample; // In this case, no need for mapping since it's already 0-255

      // Output the mapped sample to the DAC pin
      dacWrite(audioPin, dacValue); // Output the audio sample to the DAC pin

    } else {
      wav->stop();
      delete wav;
      wav = nullptr;
      Serial.println("Audio playback finished.");
    }
  }
}
