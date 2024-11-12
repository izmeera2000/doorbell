#include <HardwareSerial.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoWebsockets.h>
#include <driver/i2s.h>



const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

HardwareSerial mySerial(1);


#define I2S_MIC_PIN_CLK 14
#define I2S_MIC_PIN_DATA 32
#define I2S_SPEAKER_PIN_OUT 25  // Connect to speaker amplifier

void setup() {
  Serial.begin(115200);                        // Debug
  mySerial.begin(115200, SERIAL_8N1, 16, 17);  // RX, TX on pins 16, 17

  Serial.println("Waiting for ESP32-CAM IP...");


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");



  // Initialize I2S for audio input and output
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  // I2S Pin configuration
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_PIN_CLK,
    .ws_io_num = -1,
    .data_out_num = I2S_SPEAKER_PIN_OUT,
    .data_in_num = I2S_MIC_PIN_DATA
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);

  // Start WebSocket server
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();


  if (mySerial.available()) {
    String camIp = mySerial.readStringUntil('\n');
    if (camIp.startsWith("IP:")) {
      Serial.println("ESP32-CAM IP Address: " + camIp.substring(3));
    }
  }
}



void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    // Audio data received from Flutter app
    i2s_write(I2S_NUM_0, data, len, &len, portMAX_DELAY);
  }
}

void loop() {
  uint8_t audioBuffer[1024];
  size_t bytesRead;

  // Capture audio from microphone and send to Flutter app
  i2s_read(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
  if (bytesRead > 0) {
    ws.binaryAll(audioBuffer, bytesRead);
  }

  ws.cleanupClients();
}
