#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";         // Replace with your Wi-Fi SSID
const char *password = "Alamak323";  // Replace with your Wi-Fi Password

AsyncWebServer Audioserver(82);


//---- I2S Configuration ------------

// I2S pins
#define I2S_WS 22
#define I2S_SCK 26
#define I2S_SD 21
//CAMERA_MODEL_XIAO_ESP32S3
//#define I2S_WS            42
//#define I2S_SCK           -1
//#define I2S_SD            41

// I2S peripheral to use (0 or 1)
#define I2S_PORT I2S_NUM_1
//CAMERA_MODEL_XIAO_ESP32S3
//#define I2S_PORT          I2S_NUM_0

//---- Sampling ------------
#define SAMPLE_RATE 22050  // Sample rate of the audio
#define SAMPLE_BITS 32     // Bits per sample of the audio
//CAMERA_MODEL_XIAO_ESP32S3
//#define SAMPLE_BITS       16
#define DMA_BUF_COUNT 2
#define DMA_BUF_LEN 1024

const int sampleRate = SAMPLE_RATE;     // Sample rate of the audio
const int bitsPerSample = SAMPLE_BITS;  // Bits per sample of the audio
const int numChannels = 1;              // Number of audio channels (1 for mono, 2 for stereo)
const int bufferSize = DMA_BUF_LEN;     // Buffer size for I2S data transfer

struct WAVHeader {
  char chunkId[4];         // 4 bytes
  uint32_t chunkSize;      // 4 bytes
  char format[4];          // 4 bytes
  char subchunk1Id[4];     // 4 bytes
  uint32_t subchunk1Size;  // 4 bytes
  uint16_t audioFormat;    // 2 bytes
  uint16_t numChannels;    // 2 bytes
  uint32_t sampleRate;     // 4 bytes
  uint32_t byteRate;       // 4 bytes
  uint16_t blockAlign;     // 2 bytes
  uint16_t bitsPerSample;  // 2 bytes
  char subchunk2Id[4];     // 4 bytes
  uint32_t subchunk2Size;  // 4 bytes
};

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

 

 
  // Audio streaming endpoint
  Audioserver.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
      mic_i2s_init();

  WAVHeader wavHeader;
  initializeWAVHeader(wavHeader, sampleRate, bitsPerSample, numChannels);

  // Get access to the client object
  WiFiClient Audioclient = Audioserver.client();

  // Send the 200 OK response with the headers
  Audioclient.print("HTTP/1.1 200 OK\r\n");
  Audioclient.print("Content-Type: audio/wav\r\n");
  Audioclient.print("Access-Control-Allow-Origin: *\r\n");
  Audioclient.print("\r\n");

  // Send the initial part of the WAV header
  Audioclient.write(reinterpret_cast<const uint8_t *>(&wavHeader), sizeof(wavHeader));

  uint8_t buffer[bufferSize];
  size_t bytesRead = 0;
  //uint32_t totalDataSize = 0; // Total size of audio data sent

  while (true) {
    if (!Audioclient.connected()) {
      //i2s_driver_uninstall(I2S_PORT);
      Serial.println("Audioclient disconnected");
      break;
    }
    // Read audio data from I2S DMA
    i2s_read(I2S_PORT, buffer, bufferSize, &bytesRead, portMAX_DELAY);

    // Send audio data
    if (bytesRead > 0) {
      Audioclient.write(buffer, bytesRead);
      //totalDataSize += bytesRead;
      //Serial.println(totalDataSize);
    }
  }
  
   });

  Audioserver.begin();
}

void loop() {
 
}




void initializeWAVHeader(WAVHeader &header, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {

  strncpy(header.chunkId, "RIFF", 4);
  strncpy(header.format, "WAVE", 4);
  strncpy(header.subchunk1Id, "fmt ", 4);
  strncpy(header.subchunk2Id, "data", 4);

  header.chunkSize = 0;       // Placeholder for Chunk Size (to be updated later)
  header.subchunk1Size = 16;  // PCM format size (constant for uncompressed audio)
  header.audioFormat = 1;     // PCM audio format (constant for uncompressed audio)
  header.numChannels = numChannels;
  header.sampleRate = sampleRate;
  header.bitsPerSample = bitsPerSample;
  header.byteRate = (sampleRate * bitsPerSample * numChannels) / 8;
  header.blockAlign = (bitsPerSample * numChannels) / 8;
  header.subchunk2Size = 0;  // Placeholder for data size (to be updated later)
}


void mic_i2s_init() {

  i2s_config_t i2sConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),  // Use RX mode for audio input
    //CAMERA_MODEL_XIAO_ESP32S3
    //.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // Mono audio
    //CAMERA_MODEL_XIAO_ESP32S3
    //.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = true
  };
  i2s_driver_install(I2S_PORT, &i2sConfig, 0, NULL);

  i2s_pin_config_t pinConfig = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pinConfig);
}


void handleAudioStream() {

 
}
