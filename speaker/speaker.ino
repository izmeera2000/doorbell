#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceBuffer.h"

// Wi-Fi Credentials
const char *ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char *password = "Alamak323";   // Replace with your Wi-Fi Password

AsyncWebServer server(82);

// Audio objects
AudioGeneratorWAV *wav;
AudioOutputI2S *out;
AudioFileSourceBuffer *audioBuffer;

// Buffer for incoming audio data
#define BUFFER_SIZE 1024
uint8_t audioDataBuffer[BUFFER_SIZE];
volatile size_t audioDataLen = 0; // Keeps track of how much data is in the buffer

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure DAC output (GPIO25)
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.2);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // Use GPIO25 for DAC (BCK and WS set to 0)

  // Initialize the audio buffer with only the buffer (not the size)
  audioBuffer = new AudioFileSourceBuffer(audioDataBuffer);
  wav = new AudioGeneratorWAV();
  if (!wav->begin(audioBuffer, out)) {
    Serial.println("Failed to start WAV decoder!");
    while (true);
  }

  // Set up the server to handle the `/audio` endpoint
  server.on("/audio", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Audio received!");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Append received data to the buffer
    if (audioDataLen + len <= BUFFER_SIZE) {
      memcpy(audioDataBuffer + audioDataLen, data, len);
      audioDataLen += len;
    } else {
      Serial.println("Buffer overflow, dropping audio data.");
    }

    // Start playing the audio
    if (!wav->isRunning()) {
      wav->begin(audioBuffer, out);
    }
  });

  // Start the server
  server.begin();
  Serial.println("Server started on port 82");
}

void loop() {
  // Process the WAV audio stream
  if (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      Serial.println("Audio stream ended.");
    }
  } else if (audioDataLen > 0) {
    // Restart audio generator if buffer contains data
    wav->begin(audioBuffer, out);
    Serial.println("Restarting audio playback.");
  }
}
