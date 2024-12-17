#include "driver/i2s.h"

void i2sInit() {
    i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = 27,
        .ws_io_num = 25,
        .data_out_num = 23,
        .data_in_num = -1   // Not used
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
}

void i2sTestTone() {
    size_t bytes_written;
    const int sample_rate = 44100;
    const int frequency = 440;  // Test tone: 440Hz (A4 note)
    int16_t sample = 0;
    int amplitude = 10000;

    for (int i = 0; i < sample_rate; i++) {
        sample = amplitude * sin(2 * M_PI * frequency * i / sample_rate);
        i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bytes_written, portMAX_DELAY);
    }
}

void setup() {
    Serial.begin(115200);
    i2sInit();
    Serial.println("Playing test tone...");
}

void loop() {
    i2sTestTone();  // Repeatedly send tone
}
