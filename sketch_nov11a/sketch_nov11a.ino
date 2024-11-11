#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

const char* ssid = "your-SSID";
const char* password = "your-PASSWORD";

AsyncWebServer server(80);

// I2S configuration
#define I2S_WS 2
#define I2S_SD 15
#define I2S_SCK 14

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, 16000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  i2sInit();

  // Set up server
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Audio endpoint running...");
  });
  server.begin();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  uint8_t audioBuffer[1024];
  size_t bytesRead;

  // Read audio data from the microphone
  i2s_read(I2S_NUM_0, &audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);

  // Print sample data for debugging
  for (int i = 0; i < 10; i++) {
    Serial.print(audioBuffer[i]);
    Serial.print(" ");
  }
  Serial.println();
}
