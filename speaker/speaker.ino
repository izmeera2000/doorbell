#include <WiFi.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// WiFi credentials
const char *ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char *password = "Alamak323";   // Replace with your Wi-Fi Password

// Audio stream URL
const char* streamURL = "https://audio-edge-jfbmv.sin.d.radiomast.io/ref-128k-mp3-stereo";

// Audio objects
AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioOutputI2S *out;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
  }
  Serial.println("WiFi connected!");

  // Configure DAC output (single pin)
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.5);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // Use GPIO25 for DAC (BCK and WS set to 0)

  // Set up the HTTP audio stream
  file = new AudioFileSourceHTTPStream(streamURL);

  // Set up the MP3 decoder
  mp3 = new AudioGeneratorMP3();
  if (!mp3->begin(file, out)) {
    Serial.println("Failed to start MP3 decoder!");
    while (true); // Halt if the decoder fails to start
  }

  Serial.println("Audio streaming started...");
}

void loop() {
  // Process the MP3 audio stream
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("Audio stream ended.");
    }
  } else {
    Serial.println("Audio generator not running.");
    delay(1000);
  }
}
