#include <Arduino.h>
#include "driver/i2s.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// WiFi credentials
const char* ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char* password = "Alamak323";   // Replace with your Wi-Fi Password

// Web server and WebSocket objects
AsyncWebServer server(81);
AsyncWebSocket wsAudio("/audio");      // WebSocket for microphone audio
AsyncWebSocket wsSpeaker("/speaker");  // WebSocket for speaker audio

// Define I2S pins for INMP441 microphone
#define I2S_MIC_NUM I2S_NUM_0
#define I2S_MIC_SCK 14
#define I2S_MIC_WS 15
#define I2S_MIC_SD 32

// Define I2S pins for MAX98357A speaker
#define I2S_SPK_NUM I2S_NUM_1
#define I2S_SPK_SCK 26
#define I2S_SPK_WS 25
#define I2S_SPK_SD 33

#define SAMPLE_RATE 16000

// I2S configuration for the microphone
i2s_config_t i2s_mic_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 3,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

// I2S configuration for the speaker
i2s_config_t i2s_spk_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 8,
  .dma_buf_len = 512,
  .use_apll = false,
  .tx_desc_auto_clear = true,
  .fixed_mclk = 0
};

// Pin configuration for the microphone
i2s_pin_config_t mic_pins = {
  .bck_io_num = I2S_MIC_SCK,
  .ws_io_num = I2S_MIC_WS,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_MIC_SD
};

// Pin configuration for the speaker
i2s_pin_config_t spk_pins = {
  .bck_io_num = I2S_SPK_SCK,
  .ws_io_num = I2S_SPK_WS,
  .data_out_num = I2S_SPK_SD,
  .data_in_num = I2S_PIN_NO_CHANGE
};

void onWebSocketAudioEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    // Play received audio data on the speaker
    size_t bytesWritten;
    i2s_write(I2S_SPK_NUM, data, len, &bytesWritten, portMAX_DELAY);
  }
}

void sendMicDataToWebSocket() {
  const int bufferSize = 1024;
  int16_t micBuffer[bufferSize];
  size_t bytesRead;

  // Read audio data from the microphone
  i2s_read(I2S_MIC_NUM, &micBuffer, bufferSize * sizeof(int16_t), &bytesRead, portMAX_DELAY);

  // Send audio data to all connected WebSocket clients
  if (wsAudio.count() > 0) {
    wsAudio.binaryAll((uint8_t *)micBuffer, bytesRead);
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
  Serial.println("Connected to WiFi");

  // Configure and install I2S for the microphone
  i2s_driver_install(I2S_MIC_NUM, &i2s_mic_config, 0, NULL);
  i2s_set_pin(I2S_MIC_NUM, &mic_pins);
  i2s_set_clk(I2S_MIC_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  // Configure and install I2S for the speaker
  i2s_driver_install(I2S_SPK_NUM, &i2s_spk_config, 0, NULL);
  i2s_set_pin(I2S_SPK_NUM, &spk_pins);
  i2s_set_clk(I2S_SPK_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  // Set up WebSocket for speaker
  wsSpeaker.onEvent(onWebSocketAudioEvent);
  server.addHandler(&wsSpeaker);

  // Set up WebSocket for microphone
  server.addHandler(&wsAudio);

  // Start server
  server.begin();
  Serial.println("Server started!");
}

void loop() {
  sendMicDataToWebSocket();  // Continuously send microphone data to WebSocket
}
