#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>
 
// Wi-Fi Credentials
const char *ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char *password = "Alamak323";   // Replace with your Wi-Fi Password

AsyncWebServer server(82);

#define SAMPLE_RATE 22050
#define SAMPLE_BUFFER_SIZE 1024

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
  'R', 'I', 'F', 'F',                 // "RIFF"
  0xFF, 0xFF, 0xFF, 0x7F,             // Placeholder for ChunkSize
  'W', 'A', 'V', 'E',                 // "WAVE"
  'f', 'm', 't', ' ',                 // "fmt "
  16, 0, 0, 0,                        // Subchunk1Size (16 for PCM)
  1, 0,                               // AudioFormat (1 = PCM)
  1, 0,                               // NumChannels (1 = mono, 2 = stereo)
  (uint8_t)(SAMPLE_RATE & 0xFF),       // SampleRate (Low byte)
  (uint8_t)((SAMPLE_RATE >> 8) & 0xFF),// SampleRate (High byte)
  0x00, 0x00,                         // (This part repeats for ByteRate)
  (uint8_t)(SAMPLE_RATE & 0xFF),       // ByteRate (Low byte)
  (uint8_t)((SAMPLE_RATE >> 8) & 0xFF),// ByteRate (High byte)
  0x00, 0x00,                         // BlockAlign
  2, 0,                               // BitsPerSample (16-bit)
  16, 0,                              // Subchunk2Size (16 for PCM data)
  'd', 'a', 't', 'a',                  // "data"
  0xFF, 0xFF, 0xFF, 0x7F              // Placeholder for Subchunk2Size
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
    .intr_alloc_flags = 0,
    .dma_buf_count = 2,
    .dma_buf_len = SAMPLE_BUFFER_SIZE
    
  };

  // Initialize I2S
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pin_config);
i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);  // Set I2S clock, use APLL

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
    response->addHeader("Access-Control-Allow-Origin", "*");  // Or specify a specific domain instead of "*" if needed

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
