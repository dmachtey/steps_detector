#include "hw_mic.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "es8311.h"

static const char *TAG = "HW_MIC";

#define I2S_MCK_IO      GPIO_NUM_16
#define I2S_BCK_IO      GPIO_NUM_9
#define I2S_WS_IO       GPIO_NUM_45
#define I2S_DI_IO       GPIO_NUM_10 // RX desde ES8311
#define GPIO_OUTPUT_PA  GPIO_NUM_46 // Habilitación del Power Amp
#define TOUCH_HOST      I2C_NUM_0   // Bus I2C para configurar el ES8311

static i2s_chan_handle_t rx_handle = NULL;

void hw_mic_init(void) {
    ESP_LOGI(TAG, "Inicializando ES8311 y DMA I2S a 8 KHz...");

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PA),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_PA, 1);

    es8311_handle_t es_handle = es8311_create(TOUCH_HOST, ES8311_ADDRRES_0);
    if (es_handle) {
        const es8311_clock_config_t es_clk = {
            .mclk_inverted = false, .sclk_inverted = false,
            .mclk_from_mclk_pin = true,
            .mclk_frequency = 8000 * 384, 
            .sample_frequency = 8000      
        };
        es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
        es8311_sample_frequency_config(es_handle, 8000 * 384, 8000);
        es8311_microphone_config(es_handle, false);
        es8311_microphone_gain_set(es_handle, 4); 
        ESP_LOGI(TAG, "ES8311 configurado correctamente.");
    } else {
        ESP_LOGE(TAG, "Fallo al comunicar con el chip ES8311");
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(8000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_MCK_IO, .bclk = I2S_BCK_IO, .ws = I2S_WS_IO,
            .dout = I2S_GPIO_UNUSED, .din = I2S_DI_IO,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = 384; 

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}

bool hw_mic_read_dma(int16_t *buffer, int num_samples) {
    if (rx_handle == NULL) return false;
    size_t bytes_read = 0;
    esp_err_t ret = i2s_channel_read(rx_handle, buffer, num_samples * sizeof(int16_t), &bytes_read, portMAX_DELAY);
    return (ret == ESP_OK && bytes_read > 0);
}