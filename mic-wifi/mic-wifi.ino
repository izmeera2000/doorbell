#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

const char* ssid = "your_SSID";       // Replace with your Wi-Fi SSID
const char* password = "your_PASSWORD";  // Replace with your Wi-Fi Password

AsyncWebServer server(80);

// I2S microphone pin configuration
i2s_pin_config_t i2s_pin_config = {
  .bck_io_num = 14,    // Bit Clock
  .ws_io_num = 15,     // Word Select (LR Clock)
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = 32    // Data In (SD)
};

// WAV header template
const uint8_t wav_header[] = {
  'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
  16, 0, 0, 0, 1, 0, 1, 0, 0x80, 0x3E, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00,
  1, 0, 16, 0, 'd', 'a', 't', 'a', 0, 0, 0, 0
};

// Function to send WAV header
void sendWavHeader(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse("audio/wav", sizeof(wav_header), [request](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
    memcpy(buffer, wav_header, sizeof(wav_header));
    return sizeof(wav_header);
  });
  response->addHeader("Content-Type", "audio/wav");
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
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);

  // Endpoint to stream audio
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request){
    sendWavHeader(request);  // Send the WAV header

    // Stream audio data
    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      size_t bytesRead = 0;
      i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
      return bytesRead;
    });
    request->send(response);
  });

  server.begin();
}

void loop() {
  // Nothing needed here; the server handles requests asynchronously
}
