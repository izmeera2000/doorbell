#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>
#include <LittleFS.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;
AudioFileSourceLittleFS* fileSource = nullptr;

// Async server
AsyncWebServer server(81);

// Global file handle for writing
File audioFile;
const int buttonPin = 12;  // GPIO pin connected to the button
const char* serverUrl = "https://test.kaunselingadtectaiping.com.my/test.php";

int buttonState = 0;      // Current button state
int lastButtonState = 0;  // Previous button state
void setup() {
  // Start serial communication
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);  // Button with internal pull-up resistor

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
  }
  Serial.println("WiFi connected!");


  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Configure I2S output for audio playback
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true);  // Mono output
  out->SetGain(0.1);             // Adjust volume (0.0 to 1.0)
  out->SetPinout(0, 0, 25);      // Use GPIO25 for DAC (BCK and WS set to 0)

  // Set up HTTP POST endpoint for receiving audio
  server.on(
    "/speaker", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
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
        if (wav->begin(fileSource, out)) {
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
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = nullptr;
      Serial.println("Audio playback finished.");
    }
  }

  // Read the current state of the button
  buttonState = digitalRead(buttonPin);

  // Check if the button was pressed (HIGH to LOW transition)
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button pressed! Sending HTTP request...");

    // Send the HTTP request to the server
    sendHTTPRequest();

    // Add a small delay to debounce the button
    delay(200);
  }

  // Save the button state for the next loop iteration
  lastButtonState = buttonState;

  delay(50);  // Small delay to avoid excessive checking
}


void sendHTTPRequest() {
  // Check WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Begin the HTTP request
    http.begin(serverUrl);

    // Send the HTTP GET request (you can change this to POST if needed)
    int httpCode = http.GET();

    // Check the response
    if (httpCode > 0) {
      Serial.printf("HTTP request successful. Code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    } else {
      Serial.printf("HTTP request failed. Code: %d\n", httpCode);
    }

    // End the HTTP connection
    http.end();
  } else {
    Serial.println("Error: Not connected to WiFi");
  }
}