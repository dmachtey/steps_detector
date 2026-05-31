#include "dsp_audio.h"
#include "esp_dsp.h"
#include "esp_log.h"
#include "esp_heap_caps.h" // <-- IMPORTANTE PARA USAR LA PSRAM
#include <math.h>

#define AUDIO_SAMPLES 24000
#define FFT_SAMPLES   4096
#define SAMPLE_RATE   8000.0f

static const char *TAG = "DSP_AUDIO";

// Los cambiamos a punteros
float *fft_workspace = NULL;
float *window_hann = NULL;

esp_err_t dsp_audio_init(void) {
    // 1. Pedir memoria en la PSRAM (SPIRAM)
    fft_workspace = (float *)heap_caps_malloc(FFT_SAMPLES * 2 * sizeof(float), MALLOC_CAP_SPIRAM);
    window_hann = (float *)heap_caps_malloc(FFT_SAMPLES * sizeof(float), MALLOC_CAP_SPIRAM);

    if (fft_workspace == NULL || window_hann == NULL) {
        ESP_LOGE(TAG, "¡Error fatal! No hay PSRAM para el DSP.");
        return ESP_FAIL;
    }

    // 2. Inicializar el motor
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret == ESP_OK) {
        dsps_wind_hann_f32(window_hann, FFT_SAMPLES);
        ESP_LOGI(TAG, "Motor ESP-DSP y memoria PSRAM listos.");
    }
    return ret;
}

void dsp_audio_extraer_features(float* audio_in, float* out_rms, float* out_zcr, float* out_freq) {

  // 1. RMS (Root Mean Square) usando ESP-DSP
    float sum_sq = 0;
    // Multiplica el vector de audio por sí mismo (eleva al cuadrado) y suma todos los valores
    dsps_dotprod_f32(audio_in, audio_in, &sum_sq, AUDIO_SAMPLES);
    // Divide por la cantidad de muestras y saca la raíz cuadrada
    *out_rms = sqrtf(sum_sq / (float)AUDIO_SAMPLES);

    // 2. Cruces por Cero
    int cruces = 0;
    for(int i = 1; i < AUDIO_SAMPLES; i++) {
        if ((audio_in[i] >= 0 && audio_in[i-1] < 0) || (audio_in[i] < 0 && audio_in[i-1] >= 0)) {
            cruces++;
        }
    }
    *out_zcr = (float)cruces / (2.0f * AUDIO_SAMPLES);

    // 3. Frecuencia Dominante (FFT)
    for (int i = 0; i < FFT_SAMPLES; i++) {
        fft_workspace[i * 2] = audio_in[i] * window_hann[i];
        fft_workspace[i * 2 + 1] = 0.0f;
    }

    dsps_fft2r_fc32(fft_workspace, FFT_SAMPLES);
    dsps_bit_rev_fc32(fft_workspace, FFT_SAMPLES);
    dsps_cplx2reC_fc32(fft_workspace, FFT_SAMPLES);

    float max_amplitude = 0;
    int max_index = 0;
    for (int i = 1; i < FFT_SAMPLES / 2; i++) {
        if (fft_workspace[i] > max_amplitude) {
            max_amplitude = fft_workspace[i];
            max_index = i;
        }
    }
    *out_freq = (float)max_index * (SAMPLE_RATE / (float)FFT_SAMPLES);
}
