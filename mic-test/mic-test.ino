#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

#define WIFI_SSID "your-SSID"
#define WIFI_PASSWORD "your-PASSWORD"

// I2S configuration for INMP441 microphone
#define I2S_NUM            (I2S_NUM_0)  // Use I2S_NUM_0 or I2S_NUM_1
#define SAMPLE_RATE        (48000)      // 48kHz is typical for INMP441
#define I2S_BITS           I2S_BITS_PER_SAMPLE_16BIT
#define I2S_CHANNELS       I2S_CHANNEL_FMT_ONLY_LEFT  // Mono channel format (left)
#define I2S_MODE           (I2S_MODE_MASTER | I2S_MODE_RX)
#define I2S_PIN_BCK        26  // Clock Pin
#define I2S_PIN_WS         22  // Word Select Pin
#define I2S_PIN_SD         21  // Data Pin

// Async Web Server instance
AsyncWebServer server(82);  // Web server listens on port 80

// Buffer to store audio data
int16_t audioBuffer[1024];  // 1024 samples per buffer

// WAV Header template (for 48kHz 16-bit Mono PCM)
byte wavHeader[44] = {
  'R', 'I', 'F', 'F',    // ChunkID
  0, 0, 0, 0,            // ChunkSize (to be filled in later)
  'W', 'A', 'V', 'E',    // Format
  'f', 'm', 't', ' ',    // Subchunk1ID
  16, 0, 0, 0,           // Subchunk1Size (16 for PCM)
  1, 0,                  // AudioFormat (1 for PCM)
  1, 0,                  // NumChannels (1 for mono)
  0x80, 0xbb, 0, 0,      // SampleRate (48kHz)
  0, 0, 0, 0,            // ByteRate (SampleRate * NumChannels * BitsPerSample / 8)
  2, 0,                  // BlockAlign (NumChannels * BitsPerSample / 8)
  16, 0,                 // BitsPerSample
  'd', 'a', 't', 'a',    // Subchunk2ID
  0, 0, 0, 0             // Subchunk2Size (to be filled in later)
};

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  // I2S setup for INMP441
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)I2S_MODE,              // Correct I2S mode (Master and RX)
    .sample_rate = SAMPLE_RATE,    // 48kHz sample rate
    .bits_per_sample = I2S_BITS,   // 16-bit samples
    .channel_format = I2S_CHANNELS,  // Mono channel format
    .communication_format = I2S_COMM_FORMAT_I2S,  // I2S communication format
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Interrupt flag
    .dma_buf_count = 8,           // Number of DMA buffers
    .dma_buf_len = 1024           // Length of each buffer
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_PIN_BCK,
    .ws_io_num = I2S_PIN_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_PIN_SD
  };

  // Install I2S driver and set pins
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);

 

  // Audio streaming endpoint
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request){
    // Send headers for continuous WAV stream
    AsyncWebServerResponse *response = request->beginResponse(200, "audio/wav", nullptr);
    response->addHeader("Cache-Control", "no-cache");
    response->addHeader("Connection", "keep-alive");
    request->send(response);
    
    // Get the AsyncClient from the request
    AsyncClient *client = request->client();

    // First, send the WAV header (for 48kHz 16-bit mono PCM)
    client->write((const char*)wavHeader, sizeof(wavHeader));
    
    // Update the Subchunk2Size and ChunkSize later
    uint32_t dataSize = 0;
    
    // Start streaming audio data
    while (client->connected()) {
      size_t bytes_read;
      i2s_read(I2S_NUM, (char*)audioBuffer, sizeof(audioBuffer), &bytes_read, portMAX_DELAY);
      
      // Send audio data as chunks
      if (bytes_read > 0) {
        client->write((const char*)audioBuffer, bytes_read);
        dataSize += bytes_read;
        
        // Update WAV header (Subchunk2Size and ChunkSize)
        if (dataSize >= 1024) {  // Update once we've sent enough data
          wavHeader[40] = (dataSize & 0xFF);  // Subchunk2Size (Little Endian)
          wavHeader[41] = ((dataSize >> 8) & 0xFF);
          wavHeader[42] = ((dataSize >> 16) & 0xFF);
          wavHeader[43] = ((dataSize >> 24) & 0xFF);
          
          wavHeader[4] = ((dataSize + 36) & 0xFF);  // ChunkSize (Little Endian)
          wavHeader[5] = ((dataSize + 36) >> 8) & 0xFF;
          wavHeader[6] = ((dataSize + 36) >> 16) & 0xFF;
          wavHeader[7] = ((dataSize + 36) >> 24) & 0xFF;
          
          client->write((const char*)wavHeader, sizeof(wavHeader));  // Re-send the header with updated sizes
        }
      }
    }
  });

  // Start the WebServer
  server.begin();
}

void loop() {
  // Nothing to do here, the AsyncWebServer handles everything asynchronously
}
