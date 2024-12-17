#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>
#include <LittleFS.h>

// WiFi credentials
const char* ssid = "iPhone";       // Replace with your WiFi SSID
const char* password = "Alamak323"; // Replace with your WiFi password

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;
AudioFileSourceLittleFS* fileSource = nullptr;

// Async web server
AsyncWebServer server(81);

// Global file handle for writing
File audioFile;

void startAudioPlayback(AsyncWebServerRequest *request) {
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
  if (wav->begin(fileSource, out)) {
    Serial.println("Audio playback started");
    request->send(200, "text/plain", "Audio received and playing");
  } else {
    Serial.println("Failed to start WAV decoder!");
    request->send(500, "text/plain", "Failed to start WAV decoder");
  }
}

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
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Configure I2S output for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true);   // Set Mono output (change to false for stereo if required)
  out->SetGain(0.5);              // Adjust volume (0.0 to 1.0)
  out->SetPinout(27, 25, 23);     // BCK=GPIO27, WS=GPIO25, DATA=GPIO23
  out->SetSampleRate(44100);      // Set the sample rate (44100 Hz for standard audio)
  out->SetBitsPerSample(16);      // Set the bit depth to 16 bits (MAX98357A supports 16 bits)

  // Define HTTP POST endpoint for receiving audio
  server.on("/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Audio file uploaded successfully.");
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // File writing and chunking logic
    if (index == 0) {
      audioFile = LittleFS.open("/audio.wav", FILE_WRITE);
      if (!audioFile) {
        request->send(500, "text/plain", "Failed to open file for writing");
        return;
      }
    }
    audioFile.write(data, len);
    if (index + len == total) {
      audioFile.close();
      startAudioPlayback(request);
    }
  });

  // Start the web server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Process the audio stream (if active)
  if (wav != nullptr && wav->isRunning()) {
    Serial.println("Audio playback is running...");
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = nullptr;
      Serial.println("Audio playback finished.");
    }
  }
}
