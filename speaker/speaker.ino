#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>  // I2S for audio output

const char* ssid = "iPhone";
const char* password = "Alamak323";

AsyncWebServer server(81);

// Create objects for audio
AudioFileSourceSPIFFS fileSource;
AudioGeneratorWAV wavDecoder;
AudioOutputI2S audioOutput;  // Use I2S for audio output

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize SPIFFS (for storing files locally)
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Initialize the I2S for audio output
  audioOutput.begin();  // Uses default I2S configuration for output

  // Serve the audio stream
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Open the WAV file from SPIFFS
    File audioFile = SPIFFS.open("/audio.wav", "r");
    if (!audioFile) {
      request->send(404, "text/plain", "File Not Found");
      return;
    }

    // Set up the file source for WAV
    fileSource.open(audioFile);
    wavDecoder.begin(fileSource, audioOutput);

    // Stream and play the WAV file
    while (wavDecoder.isRunning()) {
      wavDecoder.loop(); // Decode and send audio to I2S
    }

    // Close the file after playback
    audioFile.close();
    request->send(200, "text/plain", "Audio Streaming Complete");
  });

  // Start the server
  server.begin();
}

void loop() {
  // Nothing to do in the loop, everything is handled by the server
}
