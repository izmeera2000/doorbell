#include <WiFi.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorWAV.h"   // Replace MP3 generator with WAV generator
#include "AudioOutputI2S.h"

// WiFi credentials
const char *ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char *password = "Alamak323";   // Replace with your Wi-Fi Password

// Audio stream URL (ensure it's a WAV file URL)
const char* streamURL = "http://www2.cs.uic.edu/~i101/SoundFiles/gettysburg10.wav";  // Replace with your WAV stream URL

// Audio objects
AudioGeneratorWAV *wav;               // Use WAV generator instead of MP3
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

  // Configure I2S output for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true); // Mono output
  out->SetGain(0.2);            // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);     // Use GPIO25 for DAC (BCK and WS set to 0)

  // Set up the HTTP audio stream
  file = new AudioFileSourceHTTPStream(streamURL);

  // Set up the WAV decoder
  wav = new AudioGeneratorWAV();
  if (!wav->begin(file, out)) {
    Serial.println("Failed to start WAV decoder!");
    while (true); // Halt if the decoder fails to start
  }

  Serial.println("Audio streaming started...");
}

void loop() {
  // Process the WAV audio stream
  if (wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      Serial.println("Audio stream ended.");
    }
  } else {
    Serial.println("Audio generator not running.");
    delay(1000);
  }
}
