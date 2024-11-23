#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Audio.h>
#include <SPI.h>

const char* ssid = "iPhone";
const char* password = "Alamak323";

AsyncWebServer server(80);

// Create an Audio class object to handle decoding and playback
Audio audio;
AudioFileSourceICYStream audioStream;
AudioGeneratorMP3 mp3;
AudioOutputDAC dac;  // Use internal DAC (GPIO 25)

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize SPIFFS (for storing files locally if needed)
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Initialize the Audio system (DAC output)
  audio.begin();
  audioOutput.begin();

  // Serve the audio stream
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request){
    // Open the audio file from SPIFFS (can be changed to remote streaming)
    File audioFile = SPIFFS.open("/audio.mp3", "r");
    if (!audioFile) {
      request->send(404, "text/plain", "File Not Found");
      return;
    }

    // Set up the audio generator (MP3 in this example)
    audioStream.open(audioFile);
    mp3.begin(audioStream, dac);
    
    // Play the audio
    while (mp3.isRunning()) {
      mp3.loop();
    }

    // Close the file after playback
    audioFile.close();
    request->send(200, "text/plain", "Audio Streaming Complete");
  });

  // Start the web server
  server.begin();
}

void loop() {
  // Nothing to do in the loop, everything is handled by the server
}
