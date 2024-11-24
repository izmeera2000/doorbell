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

// Initialize AsyncWebServer
AsyncWebServer server(82);

// Function to install and configure I2S
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

// Function to configure I2S pins
void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  // Start Serial monitor
  Serial.begin(115200);
  Serial.println("Starting...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi");
  Serial.println(WiFi.localIP());

  // Initialize I2S microphone
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  // Define HTTP GET endpoint to stream audio data
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Read I2S data into the buffer
    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, &audioBuffer, BUFFER_LEN, &bytesIn, portMAX_DELAY);

    if (result == ESP_OK) {
      // Send PCM audio data to the client (Flutter app)
      request->send(200, "audio/raw", (const char*)audioBuffer, bytesIn);
    } else {
      request->send(500, "text/plain", "Error reading audio data");
    }
  });

  // Start the web server
  server.begin();
}

void loop() {
  // The server handles requests asynchronously
}
