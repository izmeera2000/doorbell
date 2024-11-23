#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AudioOutputI2S.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSPIFFS.h>
#include <FS.h>
#include <SPIFFS.h>

// Wi-Fi credentials
const char *ssid = "iPhone";
const char *password = "Alamak323";

// Audio objects
AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *fileSource;
AudioOutputI2S *out;

// Web server
AsyncWebServer server(81);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi!");

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

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
        File file = SPIFFS.open("/audio.mp3", FILE_WRITE);
        if (!file) {
          Serial.println("Failed to open file for writing");
          return;
        }
        file.close();
      }

      // Append data to file
      File file = SPIFFS.open("/audio.mp3", FILE_APPEND);
      if (file) {
        file.write(data, len);
        file.close();
      }

      if (index + len == total) {  // When full audio data is received
        Serial.println("Audio data received!");
        fileSource = new AudioFileSourceSPIFFS("/audio.mp3");
        if (mp3->begin(fileSource, out)) {
          Serial.println("Playing audio...");
        } else {
          Serial.println("Failed to start audio playback");
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
