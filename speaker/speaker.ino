#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav;
AudioFileSourceHTTPStream* file;
AudioOutputI2S* out;

AsyncWebServer server(81);

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
  out->SetOutputModeMono(true);  // Mono output
  out->SetGain(0.1);             // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);      // Use GPIO25 for DAC (BCK and WS set to 0)

  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest* request) {
    Serial.println("Got request for /audio");

    // Create a buffer to store the incoming audio data
    int contentLength = request->contentLength();
    if (contentLength > 0) {
      uint8_t* audioData = new uint8_t[contentLength];

      // Read the incoming audio data
      int bytesRead = request->args();
      int index = 0;
      while (request->available()) {
        audioData[index++] = request->read();
      }

      // Ensure the correct length of the read data
      if (index != contentLength) {
        Serial.println("Error: Incorrect data length received.");
        request->send(400, "text/plain", "Failed to receive full audio data");
        return;
      }

      // Now handle the audio data (e.g., stream it to the speaker)
      Serial.println("Streaming audio data to speaker...");

      // Assuming 'audioData' now contains the raw audio data
      AudioFileSourceMemory* stream = new AudioFileSourceMemory(audioData, contentLength);
      wav = new AudioGeneratorWAV();

      if (wav->begin(stream, out)) {
        Serial.println("Audio streaming started...");
        request->send(200, "text/plain", "Audio uploaded and streaming started");
      } else {
        Serial.println("Failed to start WAV decoder!");
        request->send(500, "text/plain", "Failed to stream audio");
      }

      delete[] audioData;  // Clean up allocated memory
    } else {
      request->send(400, "text/plain", "No audio data received");
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
      Serial.println("Audio stream ended.");
    }
  }
}
