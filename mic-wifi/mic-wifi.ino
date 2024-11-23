#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <driver/i2s.h>

// Wi-Fi Credentials
const char *ssid = "iPhone";         // Replace with your Wi-Fi SSID
const char *password = "Alamak323";  // Replace with your Wi-Fi Password

AsyncWebServer server(82);

#define SAMPLE_RATE 16000    // Increase sample rate to improve quality
#define SAMPLE_BUFFER_SIZE 512
#define FILTER_SIZE 5        // Size of the filter (adjust for smoother output)

// I2S microphone pin configuration
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
#define I2S_MIC_SERIAL_CLOCK 26
#define I2S_MIC_LEFT_RIGHT_CLOCK 22
#define I2S_MIC_SERIAL_DATA 21

i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
};

// I2S configuration
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,  // Use 16-bit for reduced static
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,  // Increased buffer count
    .dma_buf_len = 2048, // Increased buffer length
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
};

// Audio buffer and filtered samples
int32_t raw_samples[SAMPLE_BUFFER_SIZE];
int32_t filtered_samples[SAMPLE_BUFFER_SIZE];
int32_t sample_buffer[FILTER_SIZE];
int filter_index = 0;

// Low-pass filter function
void applyLowPassFilter() {
    // Shift buffer and add new sample
    for (int i = 1; i < FILTER_SIZE; i++) {
        sample_buffer[i - 1] = sample_buffer[i];
    }
    sample_buffer[FILTER_SIZE - 1] = raw_samples[0]; // Raw sample data

    // Calculate average
    int32_t sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += sample_buffer[i];
    }

    // Filtered sample is the average of the buffer
    filtered_samples[0] = sum / FILTER_SIZE;
}

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

    // Start up the I2S peripheral
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);

    // Setup web server to stream audio
    server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Stream audio with low-pass filtered data
        AsyncWebServerResponse *response = request->beginChunkedResponse("audio/wav", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
            if (index == 0) {
                // Send WAV header at the beginning
                uint8_t wav_header[44] = {
                    'R', 'I', 'F', 'F', 0xFF, 0xFF, 0xFF, 0x7F, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
                    16, 0, 0, 0, 1, 0, 1, 0, (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
                    (uint8_t)(SAMPLE_RATE & 0xFF), (uint8_t)((SAMPLE_RATE >> 8) & 0xFF), 0x00, 0x00,
                    2, 0, 16, 0, 'd', 'a', 't', 'a', 0xFF, 0xFF, 0xFF, 0x7F
                };
                memcpy(buffer, wav_header, sizeof(wav_header));
                return sizeof(wav_header);
            }
            // Read and filter audio samples from I2S
            size_t bytes_read;
            i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
            int samples_read = bytes_read / sizeof(int32_t);

            // Apply low-pass filter on each sample
            for (int i = 0; i < samples_read; i++) {
                applyLowPassFilter();
                buffer[i * 2] = (filtered_samples[i] & 0xFF);         // Low byte
                buffer[i * 2 + 1] = (filtered_samples[i] >> 8) & 0xFF; // High byte
            }
            return samples_read * 2;  // Return the number of bytes written to the buffer
        });

        // Set headers to allow for streaming
        response->addHeader("Content-Type", "audio/wav");
        response->addHeader("Transfer-Encoding", "chunked");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "-1");
        request->send(response);
    });

    server.begin();
}

void loop() {
    // Feed the watchdog periodically to prevent resets
    delay(10000);  // Slow down the loop for easier debugging and stability
}
