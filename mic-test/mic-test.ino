#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

// Wi-Fi credentials
const char *ssid = "iPhone";          // Replace with your Wi-Fi SSID
const char *password = "Alamak323";   // Replace with your Wi-Fi Password

// Define I2S connections
#define I2S_WS 25
#define I2S_SD 33
#define I2S_SCK 32

// Define I2S buffer size and storage
#define BUFFER_LEN 1024
int16_t audioBuffer[BUFFER_LEN];

// Define I2S port
#define I2S_PORT I2S_NUM_0

// WAV header template (16-bit mono, 44.1kHz)
const uint8_t wav_header[] = {
  'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E',
  'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 1, 0, 0x44, 0xac, 0, 0, 0x88, 0x58, 0, 0, 1, 0, 8, 0, 0, 0,
  'd', 'a', 't', 'a', 0, 0, 0, 0
};

void i2s_install() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_LEN,
    .use_apll = false
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

AsyncWebServer server(82);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi");
  Serial.println(WiFi.localIP());

  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  // Set up server to stream audio
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Send WAV header at the beginning
    AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      if (index == 0) {
        memcpy(buffer, wav_header, sizeof(wav_header));  // Copy WAV header
        return sizeof(wav_header);
      }

      // Read audio samples from I2S
      size_t bytesRead;
      esp_err_t result = i2s_read(I2S_PORT, buffer, maxLen, &bytesRead, portMAX_DELAY);
      if (result != ESP_OK || bytesRead == 0) {
        Serial.println("I2S read error or no data.");
        return 0;  // Return 0 bytes if there's an issue
      }

      return bytesRead;  // Return the number of bytes read
    });

    // Set headers to allow for streaming
    response->addHeader("Content-Type", "audio/wav");
    response->addHeader("Transfer-Encoding", "chunked");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
  });

  // Start the web server
  server.begin();
}

void loop() {
  // Nothing to do here; the server handles the requests
}
