#include <WiFi.h>

#include <driver/i2s.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

WiFiServer server(80);

// I2S configuration for INMP441
const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
};

const i2s_pin_config_t pin_config = {
    .bck_io_num = 14,   // BCK (clock) for I2S
    .ws_io_num = 15,    // WS (word select) for I2S
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 32   // Data in from INMP441
};

// WAV header function
void sendWavHeader(WiFiClient &client, int sampleRate, int bitsPerSample, int channels) {
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    int blockAlign = channels * bitsPerSample / 8;
    char header[44] = {
        'R', 'I', 'F', 'F',  // Chunk ID
        0, 0, 0, 0,          // Chunk Size (to be filled later)
        'W', 'A', 'V', 'E',  // Format
        'f', 'm', 't', ' ',  // Subchunk1 ID
        16, 0, 0, 0,         // Subchunk1 Size
        1, 0,                // Audio Format (1 = PCM)
        (char)channels, 0,   // Num Channels
        (char)(sampleRate & 0xff), (char)((sampleRate >> 8) & 0xff), (char)((sampleRate >> 16) & 0xff), (char)((sampleRate >> 24) & 0xff),  // Sample Rate
        (char)(byteRate & 0xff), (char)((byteRate >> 8) & 0xff), (char)((byteRate >> 16) & 0xff), (char)((byteRate >> 24) & 0xff),  // Byte Rate
        (char)blockAlign, 0,  // Block Align
        (char)bitsPerSample, 0,  // Bits per Sample
        'd', 'a', 't', 'a',  // Subchunk2 ID
        0, 0, 0, 0           // Subchunk2 Size (to be filled later)
    };
    client.write((const uint8_t *)header, 44);
}

void setup() {
    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Start server
    server.begin();

    // Initialize I2S
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

void loop() {
    WiFiClient client = server.available();  // Check if a client has connected

    if (client) {
        Serial.println("Client connected!");
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: audio/wav");
        client.println("Connection: close");
        client.println();

        // Send WAV header
        sendWavHeader(client, 16000, 16, 1);

        // Stream audio data
        while (client.connected()) {
            uint8_t i2s_data[512];
            size_t bytes_read;

            // Read audio data from I2S
            i2s_read(I2S_NUM_0, &i2s_data, sizeof(i2s_data), &bytes_read, portMAX_DELAY);

            // Send audio data to client
            client.write(i2s_data, bytes_read);
        }

        client.stop();
        Serial.println("Client disconnected.");
    }
}
