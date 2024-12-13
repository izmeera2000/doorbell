#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

const char *ssid = "iPhone";         // Replace with your Wi-Fi SSID
const char *password = "Alamak323";  // Replace with your Wi-Fi Password

AsyncWebServer server(82);
AsyncWebSocket ws("/audio");

#define SAMPLE_RATE 8000
#define SAMPLE_BUFFER_SIZE 1024  // Reduced buffer size for stability
#define DMA_BUF_COUNT 3          // Reduced buffer count

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

uint8_t wav_header[44] = {
  'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 
  16, 0, 0, 0, 1, 0, 1, 0, (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00, 
  (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00, 
  2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F
};

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket Client connected");
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket Client disconnected");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi connection...");
    retries++;
    if (retries > 10) {
      Serial.println("Failed to connect to WiFi, rebooting...");
      ESP.restart();  // Reboot if Wi-Fi connection fails
    }
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // I2S config
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_IRAM,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = SAMPLE_BUFFER_SIZE
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.println("I2S driver installation failed");
    return;
  }

  err = i2s_set_pin(I2S_NUM_0, &i2s_pin_config);
  if (err != ESP_OK) {
    Serial.println("I2S pin setup failed");
    return;
  }

  // Audio streaming endpoint
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Send WAV header at the beginning
    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      static size_t retryCount = 0;
      if (index == 0) {
        // Send WAV header at the start
        memcpy(buffer, wav_header, sizeof(wav_header));
        return sizeof(wav_header);
      }

      // Read audio samples from I2S
      size_t bytesRead = 0;
      esp_err_t result = i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, 100 / portTICK_PERIOD_MS);

      // Retry on failure
      if (result != ESP_OK || bytesRead == 0) {
        if (retryCount < 3) {
          retryCount++;
          delay(10);  // Short delay before retrying
          return 0;   // Retry
        }
        Serial.println("I2S read failed after retries");
        return 0;  // No data available
      }

      retryCount = 0;  // Reset retry count on success
      return bytesRead;
    });

    response->addHeader("Content-Type", "audio/wav");
    response->addHeader("Transfer-Encoding", "chunked");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
  });

  // Start the server
  server.begin();
}

void loop() {
  yield(); // Feed the watchdog timer
  delay(100);  // Small delay to prevent watchdog reset

  // Monitor memory usage
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());

  // Capture audio and send over WebSocket
  size_t bytesRead = 0;
  uint8_t buffer[1024];
  esp_err_t result = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
  if (result == ESP_OK && bytesRead > 0) {
    // Send binary audio data to all connected WebSocket clients
    ws.textAll("Sending audio data..."); // Optional message
    ws.binaryAll(buffer, bytesRead);  // Send audio data as binary
  }
}
