#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioGeneratorMP3.h> // Use MP3 decoder
#include <AudioOutputDAC.h>   // Output to ESP32 DAC (GPIO 25)

const char* ssid = "iPhone";
const char* password = "Alamak323";

AsyncWebServer server(81);

// Create objects for audio decoding and output
AudioFileSourceSPIFFS fileSource;
AudioGeneratorMP3 mp3Decoder;  // Use MP3 decoder (could use WAV if needed)
AudioOutputDAC audioOutput;   // Use ESP32 DAC output (GPIO 25)

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize SPIFFS for storing files locally
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Initialize the DAC for audio output
  audioOutput.begin(); // Uses GPIO 25 for DAC1 output

  // Serve the audio stream
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Open the MP3 or WAV file from SPIFFS
    File audioFile = SPIFFS.open("/audio.mp3", "r");  // Change file name if necessary
    if (!audioFile) {
      request->send(404, "text/plain", "File Not Found");
      return;
    }

    // Set up the file source for decoding
    fileSource.open(audioFile);
    mp3Decoder.begin(fileSource, audioOutput);

    // Start decoding and playing the audio
    while (mp3Decoder.isRunning()) {
      mp3Decoder.loop(); // Decode and send audio to DAC
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
