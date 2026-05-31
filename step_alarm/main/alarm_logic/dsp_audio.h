#ifndef DSP_AUDIO_H
#define DSP_AUDIO_H

#include "esp_err.h"

// Inicializa el motor de la Transformada de Fourier
esp_err_t dsp_audio_init(void);

// Recibe 24000 muestras crudas y devuelve las 3 características mágicas
void dsp_audio_extraer_features(float* audio_in, float* out_rms, float* out_zcr, float* out_freq);

#endif