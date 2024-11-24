#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";         // Replace with your Wi-Fi SSID
const char *password = "Alamak323";  // Replace with your Wi-Fi Password

AsyncWebServer server(82);

#define SAMPLE_RATE 8000
#define SAMPLE_BUFFER_SIZE 128  // Reduced buffer size for stability

// I2S microphone pin configuration
#define I2S_MIC_SERIAL_CLOCK 26
#define I2S_MIC_LEFT_RIGHT_CLOCK 22
#define I2S_MIC_SERIAL_DATA 21

i2s_pin_config_t i2s_pin_config = {
  .bck_io_num = I2S_MIC_SERIAL_CLOCK,
  .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_MIC_SERIAL_DATA
};

// Modified WAV header for infinite stream
uint8_t wav_header[44] = {
  'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
  16, 0, 0, 0, 1, 0, 1, 0, (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
  (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
  2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F
};

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi connection
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // I2S config
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 10,
    .dma_buf_len = SAMPLE_BUFFER_SIZE
  };

  // Initialize I2S driver
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.println("I2S driver installation failed");
    return;
  }

  // Initialize I2S pins
  err = i2s_set_pin(I2S_NUM_0, &i2s_pin_config);
  if (err != ESP_OK) {
    Serial.println("I2S pin setup failed");
    return;
  }

  // Audio streaming endpoint
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Send WAV header at the beginning
    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      if (index == 0) {
        memcpy(buffer, wav_header, sizeof(wav_header));
        return sizeof(wav_header);
      }

      // Read audio samples from I2S
      size_t bytesRead;
      esp_err_t result = i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
      if (result != ESP_OK || bytesRead == 0) {
        Serial.println("I2S read error or no data.");
        return 0;  // Return 0 bytes if there's an issue
      }

      return bytesRead;
    });

    // Set headers to allow for streaming
    response->addHeader("Content-Type", "audio/wav");
    response->addHeader("Transfer-Encoding", "chunked");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
  });


  server.on(
    "/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      Serial.printf("Chunk received: Index=%d, Len=%d, Total=%d\n", index, len, total);

      // if (index == 0) {
      //   Serial.println("Receiving audio...");
      //   File file = LittleFS.open("/audio.wav", FILE_WRITE);
      //   if (!file) {
      //     request->send(500, "Failed to open file for writing");
      //     return;
      //   }
      //   file.write(data, len);
      //   file.close();
      // }

      // if (index + len == total) {
      //   Serial.println("Playing audio through DAC");
      //   File audioFile = LittleFS.open("/audio.wav", "r");
      //   if (!audioFile) {
      //     request->send(500, "Failed to open audio file");
      //     return;
      //   }

      //   while (audioFile.available()) {
      //     uint8_t sample = audioFile.read();  // Read 8-bit audio sample
      //     dacWrite(DAC_PIN, sample);          // Write to DAC
      //     delayMicroseconds(62);              // Adjust timing for sample rate
      //   }

      //   audioFile.close();
      //   request->send(200, "text/plain", "Audio played");
      }
    });

  // Start the server
  server.begin();
}

void loop() {
  yield();     // Feed the watchdog timer
  delay(100);  // Small delay to prevent watchdog reset

  // Monitor memory usage
  // Serial.print("Free heap: ");
  // Serial.println(ESP.getFreeHeap());

  // Additional debug if needed
  // Uncomment to view more details about Wi-Fi and server status
  // Serial.println(WiFi.localIP());
}
