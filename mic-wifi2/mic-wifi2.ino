#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

// WiFi credentials
const char *ssid = "iPhone";         // Replace with your Wi-Fi SSID
const char *password = "Alamak323";  // Replace with your Wi-Fi Password
// I2S configuration
#define I2S_NUM         I2S_NUM_0
#define I2S_BCK_PIN     26
#define I2S_WS_PIN      22
#define I2S_DO_PIN      21
#define SAMPLE_RATE     16000

// Web server and WebSocket
AsyncWebServer server(82);
AsyncWebSocket ws("/speaker");

void setupI2S() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = true
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCK_PIN,
      .ws_io_num = I2S_WS_PIN,
      .data_out_num = I2S_DO_PIN,
      .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Initialize I2S
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM);
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_BINARY) {
      // Write binary audio data to I2S
      size_t bytes_written;
      i2s_write(I2S_NUM, data, len, &bytes_written, portMAX_DELAY);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");

  // Initialize I2S
  setupI2S();

  // Set up WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // Start the server
  server.begin();
}

void loop() {
  // Handle WebSocket
  ws.cleanupClients();
}
