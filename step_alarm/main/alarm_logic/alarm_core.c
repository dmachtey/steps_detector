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
#define DIV_IMU 1000.0f
#define DIV_AUDIO 7000.0f
#define CLAMP(val) ((val) > 1.0f ? 1.0f : ((val) < -1.0f ? -1.0f : (val)))

static const char *TAG = "ALARM_CORE";

// Estructura y Colas importadas de app_core.c
typedef struct { int16_t x; int16_t y; int16_t z; } imu_data_t;
extern QueueHandle_t alarm_imu_queue;
extern QueueHandle_t alarm_mic_queue;

// Buffers para el Machine Learning
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
    int vueltas_a_dar = 150; // Para juntar los primeros 3 segundos enteros
    int audio_idx = 0;
    int imu_idx = 0;

    while (1) {
        if (!alarma_activa) {
            // Vaciar las colas para que no queden datos viejos atascados
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
            // --- SLIDING WINDOW (Desplazamos el tiempo) ---
            int offset_imu = 50 * 3;
            int offset_audio = 50 * 160;

            memmove(&features[0], &features[offset_imu], (450 - offset_imu) * sizeof(float));
            memmove(&audio_buffer[0], &audio_buffer[offset_audio], (24000 - offset_audio) * sizeof(float));

            imu_idx = 450 - offset_imu;
            audio_idx = 24000 - offset_audio;
            vueltas_a_dar = 50; // Solo juntamos 1 segundo nuevo
        }

        // --- BUCLE DE CONSUMO Y NORMALIZACIÓN ---
        // --- FILTRO DC BLOCKER (Solo para el Acelerómetro) ---
        static float dc_x = 0, dc_y = 0, dc_z = 0;
        const float ALPHA = 0.05f; // Adaptación del 5% por muestra para absorber la gravedad

        // --- BUCLE DE CONSUMO Y NORMALIZACIÓN ---
        for (int i = 0; i < vueltas_a_dar; i++) {

            // 1. Acelerómetro: Leer de Queue y Eliminar Gravedad
            imu_data_t imu_rx;
            if (xQueueReceive(alarm_imu_queue, &imu_rx, pdMS_TO_TICKS(100)) == pdTRUE) {

                // 1A. El filtro aprende cuánto pesa la gravedad actual
                dc_x = (ALPHA * imu_rx.x) + ((1.0f - ALPHA) * dc_x);
                dc_y = (ALPHA * imu_rx.y) + ((1.0f - ALPHA) * dc_y);
                dc_z = (ALPHA * imu_rx.z) + ((1.0f - ALPHA) * dc_z);

                // 1B. Restamos la gravedad y normalizamos (¡Acá nace la señal pura!)
                features[imu_idx++] = CLAMP(((float)imu_rx.x - dc_x) / DIV_IMU);
                features[imu_idx++] = CLAMP(((float)imu_rx.y - dc_y) / DIV_IMU);
                features[imu_idx++] = CLAMP(((float)imu_rx.z - dc_z) / DIV_IMU);
            } else {
                if (imu_idx >= 3) {
                    features[imu_idx] = features[imu_idx-3];
                    features[imu_idx+1] = features[imu_idx-2];
                    features[imu_idx+2] = features[imu_idx-1];
                } else {
                    features[imu_idx] = 0.0f; features[imu_idx+1] = 0.0f; features[imu_idx+2] = 0.0f;
                }
                imu_idx += 3;
            }

            // 2. Audio: Leer de Queue y Normalizar
            int16_t mic_chunk[160];
            if (xQueueReceive(alarm_mic_queue, mic_chunk, pdMS_TO_TICKS(100)) == pdTRUE) {
                for(int j = 0; j < 160; j++) {
                    audio_buffer[audio_idx++] = CLAMP((float)mic_chunk[j] / DIV_AUDIO);
                }
            }
        } // Fin del for

        // --- EXTRACCIÓN Y PREDICCIÓN ---
        // FIX: Llamada corregida con 4 argumentos como espera el header
        dsp_audio_extraer_features(audio_buffer, &features[450], &features[451], &features[452]);

        ESP_LOGI(TAG, "Evaluando Ventana - Acel_X(G): %.2f | Audio RMS: %.4f", features[0], features[450]);

        int prediccion = predict(features);

        if (prediccion == 1) {
            ESP_LOGW(TAG, "⚠️ ¡ALERTA! PASOS DETECTADOS ⚠️");
            gui_trigger_alarma_visual();
            primer_llenado = true;
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
