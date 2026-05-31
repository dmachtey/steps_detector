#include "alarm_core.h"
#include "dsp_audio.h"
#include "Classifier.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

// --- Tus librerías de Hardware reales ---
#include "../hardware/hw_imu.h"
#include "../hardware/hw_mic.h"
#include "../gui/gui.h"

static const char *TAG = "ALARM_CORE";

// Buffers
static float features[453];
static float *audio_buffer = NULL;

static void alarm_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de vigilancia iniciada. Analizando en tiempo real...");

    while (1) {
        // -------------------------------------------------------------
        // PASO 1: RECOLECTAR DATOS DE SENSORES (Metrónomo DMA)
        // -------------------------------------------------------------
        int audio_idx = 0;
        int imu_idx = 0;

        // Damos 150 vueltas. Gracias a que el DMA del I2S tarda 20ms en
        // darnos 160 muestras, este for durará exactamente 3 segundos.
        for (int i = 0; i < 150; i++) {

            // 1A. Leer 1 muestra del Acelerómetro (IMU)
            int16_t ax = 0, ay = 0, az = 0;
            if (hw_imu_read(&ax, &ay, &az)) {
                features[imu_idx++] = (float)ax;
                features[imu_idx++] = (float)ay;
                features[imu_idx++] = (float)az;
            } else {
                // Si falla el I2C momentáneamente, rellenamos con 0 para no desfasar
                features[imu_idx++] = 0.0f;
                features[imu_idx++] = 0.0f;
                features[imu_idx++] = 0.0f;
            }

            // 1B. Leer 160 muestras de audio I2S
            int16_t mic_chunk[160];
            if (hw_mic_read_dma(mic_chunk, 160)) {
                // Pasamos las muestras crudas (int16) al buffer gigante de la PSRAM (float)
                for(int j = 0; j < 160; j++) {
                    audio_buffer[audio_idx++] = (float)mic_chunk[j];
                }
            }
        }

        // -------------------------------------------------------------
        // PASO 2: EXTRAER CARACTERÍSTICAS DEL AUDIO (DSP)
        // -------------------------------------------------------------
        float f_rms = 0, f_zcr = 0, f_freq = 0;
        dsp_audio_extraer_features(audio_buffer, &f_rms, &f_zcr, &f_freq);

        features[450] = f_rms;
        features[451] = f_zcr;
        features[452] = f_freq;

        // -------------------------------------------------------------
        // PASO 3: INFERENCIA (Cerebro Random Forest)
        // -------------------------------------------------------------
        int prediccion = predict(features);

        // -------------------------------------------------------------
        // PASO 4: ACTUAR SEGÚN EL RESULTADO
        // -------------------------------------------------------------
        if (prediccion == 1) {
            ESP_LOGW(TAG, "⚠️ ¡ALERTA! PASOS DETECTADOS EN LA ESTRUCTURA ⚠️");
            // Llamamos a la GUI para que se ponga en rojo por 30 segundos
            gui_trigger_alarma_visual();

            // ---> ACÁ VA TU ACCIÓN FÍSICA <---
            // (Ej: gpio_set_level(PIN_RELE, 1); o mandar alerta por red)

        } else {
            // ESP_LOGD(TAG, "Agua/viento normal."); // Comentado para no spamear la consola
        }
    }
}

void alarm_core_start_task(void) {
    // 1. Pedir memoria en la PSRAM para no asfixiar a LVGL
    audio_buffer = (float *)heap_caps_malloc(24000 * sizeof(float), MALLOC_CAP_SPIRAM);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Error: No hay PSRAM disponible para el buffer de audio.");
        return;
    }

    // 2. Inicializar la matemática DSP
    if (dsp_audio_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando DSP.");
        return;
    }

    // 3. Crear la tarea (sin iniciar el hardware porque ya lo hace tu hardware_init())
    xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 5, NULL);
}
