#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceLittleFS.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";
const char *password = "Alamak323";

// Async server
AsyncWebServer server(82);

// Global variables
AudioGeneratorWAV *wav = nullptr;
AudioFileSourceLittleFS *fileSource = nullptr;

// WAV header for microphone streaming
uint8_t wav_header[44] = {
    'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
    16, 0, 0, 0, 1, 0, 1, 0, 0x44, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F};

// Endpoint for microphone streaming
server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    if (index == 0) {
      memcpy(buffer, wav_header, sizeof(wav_header));
      return sizeof(wav_header);
    }

    size_t bytesRead;
    // Replace with your microphone I2S read code
    // i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
    return bytesRead;
  });
  request->send(response);
});

// Endpoint for speaker playback
server.on("/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    Serial.println("Receiving audio...");
    File file = LittleFS.open("/audio.wav", FILE_WRITE);
    if (!file) {
      request->send(500, "Failed to open file for writing");
      return;
    }
    file.write(data, len);
    file.close();
  }

  if (index + len == total) {
    Serial.println("Playback started");
    fileSource = new AudioFileSourceLittleFS("/audio.wav");
    wav = new AudioGeneratorWAV();
    
    if (wav->begin(fileSource, nullptr)) {
      request->send(200, "text/plain", "Audio playing");
    } else {
      request->send(500, "text/plain", "Failed to play audio");
    }
  }
});

server.begin();

void loop() {
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = nullptr;
      delete fileSource;
      fileSource = nullptr;
    }
  }
  
  // Output audio data to DAC (e.g., GPIO 25) if audio is being played
  if (wav && wav->isRunning()) {
    int16_t sample = wav->read();
    if (sample >= 0) {
      // Map sample to DAC value
      uint8_t dacOutput = map(sample, -32768, 32767, 0, 255);
      dacWrite(25, dacOutput);  // Use DAC1 (GPIO 25)
    }
  }
}
