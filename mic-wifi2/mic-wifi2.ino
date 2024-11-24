#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>
#include <LittleFS.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";
const char *password = "Alamak323";

// Async server on port 80
AsyncWebServer server(82);

// I2S Microphone Configuration
#define SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE 512
#define I2S_MIC_SERIAL_CLOCK 26
#define I2S_MIC_LEFT_RIGHT_CLOCK 22
#define I2S_MIC_SERIAL_DATA 21

// DAC pin for Speaker
#define DAC_PIN 25

// WAV header for streaming
uint8_t wav_header[44] = {
    'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
    16, 0, 0, 0, 1, 0, 1, 0, (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
    (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
    2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F};

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to initialize LittleFS");
    return;
  }

  // Configure Microphone I2S
  i2s_config_t mic_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = SAMPLE_BUFFER_SIZE,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t mic_pins = {
      .bck_io_num = I2S_MIC_SERIAL_CLOCK,
      .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_MIC_SERIAL_DATA};

  i2s_driver_install(I2S_NUM_0, &mic_config, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &mic_pins);

  // Endpoint for streaming microphone audio
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      if (index == 0) {
        memcpy(buffer, wav_header, sizeof(wav_header));
        return sizeof(wav_header);
      }
      size_t bytesRead;
      i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
      return bytesRead;
    });
    request->send(response);
  });

  // Endpoint for playing uploaded audio on DAC
  server.on("/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
      Serial.println("Receiving audio...");
      File file = LittleFS.open("/audio.wav", FILE_WRITE);
      if (!file) {
        request->send(500, "Failed to open file for writing");
        return;
      }
      file.write(data, len);
      file.close();
    }

    if (index + len == total) {
      Serial.println("Playing audio through DAC");
      File audioFile = LittleFS.open("/audio.wav", "r");
      if (!audioFile) {
        request->send(500, "Failed to open audio file");
        return;
      }

      while (audioFile.available()) {
        uint8_t sample = audioFile.read(); // Read 8-bit audio sample
        dacWrite(DAC_PIN, sample);         // Write to DAC
        delayMicroseconds(62);             // Adjust timing for sample rate
      }

      audioFile.close();
      request->send(200, "text/plain", "Audio played");
    }
  });

  server.begin();
}

void loop() {
  // No need for additional processing in the loop
}
