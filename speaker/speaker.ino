#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV *wav;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

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
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.2);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // Use GPIO25 for DAC (BCK and WS set to 0)

  // Set up HTTP POST endpoint for receiving audio
  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest *request){
    // Check if the request contains a file
    if (request->hasParam("file", true)) {
      // Get the file as a parameter
      const AsyncWebParameter* param = request->getParam("file", true);

      // Directly stream the audio data without storing it
      if (param->value().length() > 0) {
        Serial.println("Streaming audio data to speaker...");
        
        // Set up the WAV stream
        AudioFileSourceHTTPStream* stream = new AudioFileSourceHTTPStream(param->value().c_str());
        wav = new AudioGeneratorWAV();
        
        // Start audio playback
        if (wav->begin(stream, out)) {
          Serial.println("Audio streaming started...");
        } else {
          Serial.println("Failed to start WAV decoder!");
          request->send(500, "text/plain", "Failed to stream audio");
          return;
        }
        
        // Return success response
        request->send(200, "text/plain", "Audio uploaded and streaming started");
      } else {
        request->send(400, "text/plain", "Empty audio data");
      }
    } else {
      request->send(400, "text/plain", "No file uploaded");
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
