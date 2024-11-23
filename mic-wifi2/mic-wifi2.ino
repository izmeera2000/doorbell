#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioFileSourceLittleFS.h>
#include <LittleFS.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioFileSourceLittleFS* fileSource = nullptr;

// Async server
AsyncWebServer server(82);

// Global file handle for writing
File audioFile;

#define AUDIO_PIN 25  // GPIO pin connected to LM386

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

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Configure the audio output pin
  pinMode(AUDIO_PIN, OUTPUT); // Set the GPIO pin connected to LM386 as an output

  // Set up HTTP POST endpoint for receiving audio
  server.on("/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
              Serial.printf("Chunk received: Index=%d, Len=%d, Total=%d\n", index, len, total);

              if (index == 0) {
                Serial.println("Receiving audio data...");
                audioFile = LittleFS.open("/audio.wav", FILE_WRITE);
                if (!audioFile) {
                  Serial.println("Failed to open file for writing");
                  request->send(500, "text/plain", "Failed to open file for writing");
                  return;
                }
              }

              // Write received data to the file
              if (audioFile) {
                audioFile.write(data, len);
              }

              if (index + len == total) {
                Serial.println("Audio data received, finalizing file...");
                if (audioFile) {
                  audioFile.close();
                }

                // Stop previous playback if running
                if (wav && wav->isRunning()) {
                  wav->stop();
                  delete wav;
                  wav = nullptr;
                }
                if (fileSource) {
                  delete fileSource;
                  fileSource = nullptr;
                }

                // Initialize WAV playback
                fileSource = new AudioFileSourceLittleFS("/audio.wav");
                wav = new AudioGeneratorWAV();
                if (wav->begin(fileSource, nullptr)) {  // No I2S output, we handle audio directly
                  Serial.println("Audio playback started");
                  request->send(200, "text/plain", "Audio received and playing");
                } else {
                  Serial.println("Failed to start WAV decoder!");
                  request->send(500, "text/plain", "Failed to start WAV decoder");
                }
              }
            });

  // Start the server
  server.begin();
}

void loop() {
  // Process the audio stream (if active)
  if (wav != nullptr && wav->isRunning()) {
    if (wav->loop()) {
      // Read the next sample (mono in your case)
      int16_t sample = wav->read(); // This will give the next audio sample
      
      // Map the sample value to PWM signal
      analogWrite(AUDIO_PIN, (sample + 32768) / 256); // Adjust volume (sample ranges from -32768 to 32767)
    } else {
      wav->stop();
      delete wav;
      wav = nullptr;
      Serial.println("Audio playback finished.");
    }
  }
}


