#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Async server
AsyncWebServer server(81);

// Global file handle for writing
File audioFile;

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

  // Set up HTTP POST endpoint for receiving audio
  server.on(
    "/speaker", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
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

        request->send(200, "text/plain", "Audio received and ready to play");
      }
    });

  // Start the server
  server.begin();
}

void loop() {
  // Open the WAV file
  File audioFile = LittleFS.open("/audio.wav", "r");
  if (!audioFile) {
    Serial.println("Failed to open audio file!");
    return;
  }

  // Skip WAV header (44 bytes for standard WAV files)
  audioFile.seek(44);

  // Read and output audio data to DAC
  while (audioFile.available()) {
    int sample = audioFile.read();  // Read 1 byte (8-bit sample)
    
    if (sample >= 0) {
      // Map the sample to DAC output range (0 to 255)
      uint8_t dacOutput = (sample + 128);  // Adjust to fit 0-255 range for DAC
      dacWrite(25, dacOutput);  // Send to DAC1 (GPIO 25)
    }
  }

  // Close the file after playback
  audioFile.close();
  Serial.println("Audio playback finished.");
  delay(1000);  // Optional delay before restarting playback
}
