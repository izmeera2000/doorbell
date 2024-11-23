#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266Audio.h>  // Using ESP8266Audio library for audio playback

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV *wav;
AudioOutputI2S *out;
AudioFileSourceBuffer *audioSource = nullptr;

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
  out->SetGain(0.1);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // Use GPIO25 for DAC (BCK and WS set to 0)

  // Set up HTTP POST endpoint for receiving audio
  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest* request){
    Serial.println("Received POST request for /audio");

    int contentLength = request->contentLength();
    if (contentLength > 0) {
      // Buffer to store the audio data
      uint8_t* audioData = new uint8_t[contentLength];

      // Use request->onData() to read the incoming data in chunks
      request->onData([audioData, contentLength](const uint8_t *data, size_t len, size_t index, size_t total) {
        // Copy data to the buffer
        memcpy(audioData + index, data, len);
        if (index + len == contentLength) {
          Serial.println("Finished receiving audio data.");
          // Process the audio data after receiving all of it
          audioSource = new AudioFileSourceBuffer(audioData, contentLength);
          wav = new AudioGeneratorWAV();

          // Begin streaming the audio data
          if (wav->begin(audioSource, out)) {
            Serial.println("Audio streaming started...");
            request->send(200, "text/plain", "Audio uploaded and streaming started");
          } else {
            Serial.println("Failed to start WAV decoder!");
            request->send(500, "text/plain", "Failed to stream audio");
          }

          delete[] audioData;  // Clean up the memory
        }
      });
    } else {
      request->send(400, "text/plain", "No audio data received");
    }
  });

  // Start the server
  server.begin();
}

void loop() {
  // Process the audio stream if it is active
  if (wav != nullptr && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      Serial.println("Audio stream ended.");
    }
  }
}
