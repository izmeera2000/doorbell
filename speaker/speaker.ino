#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AudioOutputI2S.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceBuffer.h>

// Wi-Fi credentials
const char *ssid = "iPhone";
const char *password = "Alamak323";

// Audio objects
AudioGeneratorMP3 *mp3;
AudioFileSourceBuffer *fileBuffer;
AudioOutputI2S *out;

// Web server
AsyncWebServer server(81);

// Buffer for received audio data
#define AUDIO_BUFFER_SIZE 8192
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
size_t audioBufferIndex = 0;

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi!");

  // Initialize audio output (DAC output)
  out = new AudioOutputI2S(0, 1);  // Use DAC pins
  out->SetPinout(25, 26, -1);      // GPIO25 for Left, GPIO26 for Right
  out->SetOutputModeMono(true);    // Mono output for LM386
  mp3 = new AudioGeneratorMP3();

  // Start web server
  server.on(
    "/audio", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        Serial.println("Receiving audio data...");
        audioBufferIndex = 0;  // Reset buffer index
      }
      // Append data to buffer
      if (audioBufferIndex + len <= AUDIO_BUFFER_SIZE) {
        memcpy(audioBuffer + audioBufferIndex, data, len);
        audioBufferIndex += len;
      }

      if (index + len == total) {  // When full audio data is received
        Serial.println("Audio data received!");
        fileBuffer = new AudioFileSourceBuffer(audioBuffer, total);
        if (mp3->begin(fileBuffer, out)) {
          Serial.println("Playing audio...");
        } else {
          Serial.println("Failed to play audio");
        }
      }
    });

  server.begin();
  Serial.println("Server started!");
}

void loop() {
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("Audio playback finished");
    }
  }
}
