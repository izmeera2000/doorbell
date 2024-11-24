#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>
#include <LittleFS.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceLittleFS.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";
const char *password = "Alamak323";

// Async server
AsyncWebServer server(80);

// Microphone I2S configuration
#define SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE 512
#define I2S_MIC_SERIAL_CLOCK 26
#define I2S_MIC_LEFT_RIGHT_CLOCK 22
#define I2S_MIC_SERIAL_DATA 21

// Speaker I2S configuration
#define I2S_SPEAKER_CLOCK 0
#define I2S_SPEAKER_WORD_SELECT 0
#define I2S_SPEAKER_DATA 25

// Global variables
AudioGeneratorWAV *wav = nullptr;
AudioOutputI2S *audioOut = nullptr;
AudioFileSourceLittleFS *fileSource = nullptr;

// WAV header for microphone streaming
uint8_t wav_header[44] = {
    'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
    16, 0, 0, 0, 1, 0, 1, 0, (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
    (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
    2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F};

void setup() {
  Serial.begin(115200);

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Failed to initialize LittleFS");
    return;
  }

  // Initialize microphone I2S
  i2s_config_t mic_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = SAMPLE_BUFFER_SIZE,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t mic_pins = {
      .bck_io_num = I2S_MIC_SERIAL_CLOCK,
      .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_MIC_SERIAL_DATA};

  i2s_driver_install(I2S_NUM_0, &mic_config, 0, nullptr);
  i2s_set_pin(I2S_NUM_0, &mic_pins);

  // Initialize speaker I2S
  i2s_config_t speaker_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = SAMPLE_BUFFER_SIZE,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t speaker_pins = {
      .bck_io_num = I2S_SPEAKER_CLOCK,
      .ws_io_num = I2S_SPEAKER_WORD_SELECT,
      .data_out_num = I2S_SPEAKER_DATA,
      .data_in_num = I2S_PIN_NO_CHANGE};

  i2s_driver_install(I2S_NUM_1, &speaker_config, 0, nullptr);
  i2s_set_pin(I2S_NUM_1, &speaker_pins);

  // Audio playback setup
  audioOut = new AudioOutputI2S(0, 1);
  audioOut->SetPinout(I2S_SPEAKER_CLOCK, I2S_SPEAKER_WORD_SELECT, I2S_SPEAKER_DATA);

  // Endpoint for microphone streaming
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      if (index == 0) {
        memcpy(buffer, wav_header, sizeof(wav_header));
        return sizeof(wav_header);
      }

      size_t bytesRead;
      i2s_read(I2S_NUM_0, buffer, maxLen, &bytesRead, portMAX_DELAY);
      return bytesRead;
    });
    request->send(response);
  });

  // Endpoint for speaker playback
  server.on("/speaker", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
      Serial.println("Receiving audio...");
      File file = LittleFS.open("/audio.wav", FILE_WRITE);
      if (!file) {
        request->send(500, "Failed to open file for writing");
        return;
      }
      file.write(data, len);
      file.close();
    }

    if (index + len == total) {
      Serial.println("Playback started");
      fileSource = new AudioFileSourceLittleFS("/audio.wav");
      wav = new AudioGeneratorWAV();
      if (wav->begin(fileSource, audioOut)) {
        request->send(200, "text/plain", "Audio playing");
      } else {
        request->send(500, "text/plain", "Failed to play audio");
      }
    }
  });

  server.begin();
}

void loop() {
  if (wav && wav->isRunning()) {
    if (!wav->loop()) {
      wav->stop();
      delete wav;
      wav = nullptr;
      delete fileSource;
      fileSource = nullptr;
    }
  }
}
