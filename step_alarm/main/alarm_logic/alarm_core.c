#include "alarm_core.h"
#include "dsp_audio.h"
#include "Classifier.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>

#include "../gui/gui.h"

// --- CONSTANTES DE NORMALIZACIÓN (El "Punto Dulce" de Octave) ---
#define DIV_IMU 800.0f
#define DIV_AUDIO 2000.0f
#define CLAMP(val) ((val) > 1.0f ? 1.0f : ((val) < -1.0f ? -1.0f : (val)))

static const char *TAG = "ALARM_CORE";

// Estructura y Colas importadas de app_core.c
typedef struct { int16_t x; int16_t y; int16_t z; } imu_data_t;
extern QueueHandle_t alarm_imu_queue;
extern QueueHandle_t alarm_mic_queue;

// Buffers para el Machine Learning
static int16_t raw_imu_buffer[450];
static float features[453];
static float *audio_buffer = NULL;

static bool alarma_activa = false;

void alarm_core_set_state(bool state) {
    alarma_activa = state;
    if (state) {
        ESP_LOGI(TAG, "Vigilancia ACTIVADA");
    } else {
        ESP_LOGI(TAG, "Vigilancia DESACTIVADA");
    }
}

static void alarm_task(void *pvParameters) {
    ESP_LOGI(TAG, "Cerebro IA iniciado. Esperando armado...");

    bool primer_llenado = true;
    int vueltas_a_dar = 150;
    int audio_idx = 0;
    int imu_idx = 0;

    while (1) {
        if (!alarma_activa) {
            if (alarm_imu_queue != NULL) xQueueReset(alarm_imu_queue);
            if (alarm_mic_queue != NULL) xQueueReset(alarm_mic_queue);
            vTaskDelay(pdMS_TO_TICKS(200));
            primer_llenado = true;
            continue;
        }

        if (primer_llenado) {
            audio_idx = 0;
            imu_idx = 0;
            vueltas_a_dar = 150;
        } else {
            // --- SLIDING WINDOW ---
            int offset_imu = 50 * 3;
            int offset_audio = 50 * 160;

            // Desplazamos el buffer crudo del IMU y el de Audio
            memmove(&raw_imu_buffer[0], &raw_imu_buffer[offset_imu], (450 - offset_imu) * sizeof(int16_t));
            memmove(&audio_buffer[0], &audio_buffer[offset_audio], (24000 - offset_audio) * sizeof(float));

            imu_idx = 450 - offset_imu;
            audio_idx = 24000 - offset_audio;
            vueltas_a_dar = 50;
        }

        // --- BUCLE DE CONSUMO ---
        for (int i = 0; i < vueltas_a_dar; i++) {

            // 1. Acelerómetro: Solo guardamos la muestra CRUDA
            imu_data_t imu_rx;
            if (xQueueReceive(alarm_imu_queue, &imu_rx, pdMS_TO_TICKS(100)) == pdTRUE) {
                raw_imu_buffer[imu_idx++] = imu_rx.x;
                raw_imu_buffer[imu_idx++] = imu_rx.y;
                raw_imu_buffer[imu_idx++] = imu_rx.z;
            } else {
                if (imu_idx >= 3) {
                    raw_imu_buffer[imu_idx]   = raw_imu_buffer[imu_idx-3];
                    raw_imu_buffer[imu_idx+1] = raw_imu_buffer[imu_idx-2];
                    raw_imu_buffer[imu_idx+2] = raw_imu_buffer[imu_idx-1];
                } else {
                    raw_imu_buffer[imu_idx] = 0; raw_imu_buffer[imu_idx+1] = 0; raw_imu_buffer[imu_idx+2] = 0;
                }
                imu_idx += 3;
            }

            // 2. Audio: Normalizamos al vuelo (DIV_AUDIO = 7000.0)
            int16_t mic_chunk[160];
            if (xQueueReceive(alarm_mic_queue, mic_chunk, pdMS_TO_TICKS(100)) == pdTRUE) {
                for(int j = 0; j < 160; j++) {
                    audio_buffer[audio_idx++] = CLAMP((float)mic_chunk[j] / DIV_AUDIO);
                }
            }
        } // Fin del for

        // ========================================================
        // MAGIA MATEMÁTICA: Espejo 100% exacto de Python (np.mean)
        // ========================================================
        float mean_x = 0, mean_y = 0, mean_z = 0;

        // A. Calculamos el promedio exacto de los últimos 3 segundos
        for (int i = 0; i < 450; i += 3) {
            mean_x += raw_imu_buffer[i];
            mean_y += raw_imu_buffer[i+1];
            mean_z += raw_imu_buffer[i+2];
        }
        mean_x /= 150.0f;
        mean_y /= 150.0f;
        mean_z /= 150.0f;

        // B. Restamos la gravedad, dividimos (DIV_IMU = 1000.0) y saturamos
        for (int i = 0; i < 450; i += 3) {
            features[i]   = CLAMP(((float)raw_imu_buffer[i]   - mean_x) / DIV_IMU);
            features[i+1] = CLAMP(((float)raw_imu_buffer[i+1] - mean_y) / DIV_IMU);
            features[i+2] = CLAMP(((float)raw_imu_buffer[i+2] - mean_z) / DIV_IMU);
        }

        // --- EXTRACCIÓN Y PREDICCIÓN ---
        dsp_audio_extraer_features(audio_buffer, &features[450], &features[451], &features[452]);

        ESP_LOGI(TAG, "Evaluando Ventana - Acel_X(G): %.2f | Audio RMS: %.4f", features[0], features[450]);

        int prediccion = predict(features);

        if (prediccion == 1) {
            ESP_LOGW(TAG, "⚠️ ¡ALERTA! PASOS DETECTADOS ⚠️");
            gui_trigger_alarma_visual();
            primer_llenado = true; // Forzamos 3s limpios sin solapamiento
        } else {
            primer_llenado = false;
        }
    }
}

void alarm_core_start_task(void) {
    audio_buffer = (float *)heap_caps_malloc(24000 * sizeof(float), MALLOC_CAP_SPIRAM);
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Error: No hay PSRAM disponible para el buffer de audio.");
        return;
    }

    if (dsp_audio_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando DSP.");
        return;
    }

    xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 5, NULL);
}
