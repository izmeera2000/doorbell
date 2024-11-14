#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

const char *ssid = "iPhone";         // Replace with your Wi-Fi SSID
const char *password = "Alamak323";  // Replace with your Wi-Fi Password


AsyncWebServer server(80);

#define SAMPLE_RATE 8000
#define SAMPLE_BUFFER_SIZE 512

#define I2S_MIC_SERIAL_CLOCK 26
#define I2S_MIC_LEFT_RIGHT_CLOCK 22
#define I2S_MIC_SERIAL_DATA 21

i2s_pin_config_t i2s_pin_config = {
  .bck_io_num = I2S_MIC_SERIAL_CLOCK,
  .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_MIC_SERIAL_DATA
};

// WAV header
uint8_t wav_header[44] = {
  'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
  16, 0, 0, 0, 1, 0, 1, 0, 0x40, 0x1F, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00,
  2, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0
};

void updateWavHeader(uint32_t dataSize) {
  uint32_t fileSize = dataSize + 36;
  wav_header[4] = (fileSize & 0xFF);
  wav_header[5] = (fileSize >> 8) & 0xFF;
  wav_header[6] = (fileSize >> 16) & 0xFF;
  wav_header[7] = (fileSize >> 24) & 0xFF;
  wav_header[40] = (dataSize & 0xFF);
  wav_header[41] = (dataSize >> 8) & 0xFF;
  wav_header[42] = (dataSize >> 16) & 0xFF;
  wav_header[43] = (dataSize >> 24) & 0xFF;
}

void sendWavHeader(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse("audio/wav", sizeof(wav_header), [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    memcpy(buffer, wav_header, sizeof(wav_header));
    return sizeof(wav_header);
  });
  response->addHeader("Content-Type", "audio/wav");
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "-1");
  request->send(response);
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // I2S configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = SAMPLE_BUFFER_SIZE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);

  // Audio stream endpoint
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendWavHeader(request);  // Send WAV header

    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      size_t bytesRead = 0;
      i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
      return bytesRead;
    });
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
  });

  server.begin();
}

void loop() {
  // No code needed here; server handles everything asynchronously
}
