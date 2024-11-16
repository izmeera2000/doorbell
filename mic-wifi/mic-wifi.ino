#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>
 
// Wi-Fi Credentials
const char *ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char *password = "Alamak323";   // Replace with your Wi-Fi Password

AsyncWebServer server(80);

#define SAMPLE_RATE 8000
#define SAMPLE_BUFFER_SIZE 512

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
    .dma_buf_count = 10,
    .dma_buf_len = SAMPLE_BUFFER_SIZE
  };

  // Initialize I2S
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);

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
      i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
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

  server.begin();
 }

void loop() {
  // Feed the watchdog periodically to prevent resets
   
  // Monitor memory usage for debugging
  // Serial.print("Free heap: ");
  // Serial.println(ESP.getFreeHeap());
  delay(10000);
  
  // delay(1000);  // Slow down the loop for easier debugging and stability
}
