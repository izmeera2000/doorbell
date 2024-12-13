#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Alamak323";

// WebSocket server
AsyncWebServer server(82);
AsyncWebSocket ws("/audio");  // WebSocket for audio stream

// Audio objects
AudioGeneratorWAV* wav = nullptr;
AudioOutputI2S* out = nullptr;

// I2S microphone pin configuration
#define SAMPLE_RATE 8000
#define SAMPLE_BUFFER_SIZE 1024
#define DMA_BUF_COUNT 3
#define I2S_MIC_SERIAL_CLOCK 26
#define I2S_MIC_LEFT_RIGHT_CLOCK 22
#define I2S_MIC_SERIAL_DATA 21

i2s_pin_config_t i2s_pin_config = {
  .bck_io_num = I2S_MIC_SERIAL_CLOCK,
  .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_MIC_SERIAL_DATA
};

// Audio streaming buffer
uint8_t wav_header[44] = {
  'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
  16, 0, 0, 0, 1, 0, 1, 0, (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
  (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
  2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F
};

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket Client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket Client disconnected");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);  // Add WebSocket to server

  // Configure I2S for microphone input
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

  // Configure I2S for audio playback (for streaming)
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true);  // Mono output
  out->SetGain(0.5);             // Adjust volume
  out->SetPinout(14, 25, 27);    // BCK=GPIO26, WS=GPIO25, DATA=GPIO27 (modify as needed)

  // Start the server
  server.begin();
}

void loop() {
  yield();     // Feed the watchdog timer
  delay(100);  // Small delay to prevent watchdog reset

  // Capture audio and send over WebSocket to clients
  size_t bytesRead = 0;
  uint8_t buffer[1024];
  esp_err_t result = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
  if (result == ESP_OK && bytesRead > 0) {
    // Send binary audio data to all connected WebSocket clients
    ws.textAll("Sending audio data...");  // Optional message
    ws.binaryAll(buffer, bytesRead);      // Send audio data as binary
  }

  // Handle WebSocket events for receiving audio data
  // Declare audio buffer and buffer size globally
  uint8_t* audioBuffer = nullptr;
  size_t bufferSize = 0;

ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_DATA) {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;

      // Handle binary data for real-time audio streaming
      if (info->opcode == WS_BINARY) {
        Serial.printf("Received binary chunk of size %d\n", len);

        // Allocate a buffer for incoming audio data if not already allocated
        if (audioBuffer == nullptr) {
          bufferSize = len;
          audioBuffer = (uint8_t*)malloc(bufferSize);
          
          if (!audioBuffer) {
            Serial.println("Failed to allocate audio buffer!");
            client->text("Error: Unable to allocate buffer");
            return;
          }
        }

        // Copy the received data into the buffer
        memcpy(audioBuffer, data, len);

        // Process the buffer in chunks of two samples (stereo)
        for (size_t i = 0; i < len; i += sizeof(int16_t) * 2) {
          int16_t* sample = (int16_t*)(audioBuffer + i);

          // Send the stereo samples to the audio output
          if (!out->ConsumeSample(sample)) {
            Serial.println("Failed to play audio data");
          }
        }
        Serial.println("Audio data played");
      }
    }
  });
}