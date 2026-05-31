#include "alarm_core.h"
#include "dsp_audio.h"
#include "Classifier.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h" 

static const char *TAG = "ALARM_CORE";

// Features es chico (1.8 KB), puede quedar en la RAM interna
static float features[453];

// El buffer gigante lo pasamos a puntero
static float *audio_buffer = NULL;

static void alarm_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de vigilancia iniciada.");

    while (1) {
        // -------------------------------------------------------------
        // PASO 1: RECOLECTAR DATOS DE SENSORES
        // -------------------------------------------------------------
        // Aquí debes llenar tus datos reales leyendo durante 3 segundos. 
        // Asumimos que llenas el 'audio_buffer' desde el micro I2S
        // y los primeros 450 lugares de 'features' desde el acelerómetro I2C.
        
        /* EJEMPLO DE CÓMO SE LLENARÍA EL ACELERÓMETRO:
        for(int i=0; i<150; i++) {
            features[i*3]     = leer_accel_x();
            features[i*3 + 1] = leer_accel_y();
            features[i*3 + 2] = leer_accel_z();
        }
        */

        // -------------------------------------------------------------
        // PASO 2: EXTRAER CARACTERÍSTICAS DEL AUDIO (DSP)
        // -------------------------------------------------------------
        float f_rms = 0, f_zcr = 0, f_freq = 0;
        
        // Llamamos a la función que armamos en dsp_audio.c
        dsp_audio_extraer_features(audio_buffer, &f_rms, &f_zcr, &f_freq);

        // Guardamos los 3 valores mágicos del audio al final del vector 
        // (posiciones 450, 451 y 452) para completar los 453 datos.
        features[450] = f_rms;
        features[451] = f_zcr;
        features[452] = f_freq;

        // Opcional: imprimir en consola para depurar
        // ESP_LOGD(TAG, "Audio -> RMS: %.4f, ZCR: %.4f, Freq: %.1f Hz", f_rms, f_zcr, f_freq);

        // -------------------------------------------------------------
        // PASO 3: INFERENCIA (Pasar todo al Cerebro Random Forest)
        // -------------------------------------------------------------
        int prediccion = predict(features);

        // -------------------------------------------------------------
        // PASO 4: ACTUAR SEGÚN EL RESULTADO
        // -------------------------------------------------------------
        if (prediccion == 1) { // 1 = 'steps' (Según lo definimos en Python)
            ESP_LOGW(TAG, "⚠️ ¡ALERTA! PASOS DETECTADOS EN LA ESTRUCTURA ⚠️");
            // ¡Acá activás el GPIO de tu Relé, Sirena o envías el mensaje!
            
        } else {
            ESP_LOGD(TAG, "Todo tranquilo (solo ruido de agua o viento).");
        }

        // Pequeña pausa antes de iniciar el siguiente ciclo de 3 segundos
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void alarm_core_start_task(void) {
    // 1. Pedir memoria para el buffer de audio en la PSRAM externa
    audio_buffer = (float *)heap_caps_malloc(24000 * sizeof(float), MALLOC_CAP_SPIRAM);

    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Error: No hay PSRAM disponible para el buffer de audio.");
        return;
    }

    // 2. Inicializar los motores matemáticos del ESP-DSP
    if (dsp_audio_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando DSP.");
        return;
    }

    // 3. Lanzar la tarea en segundo plano con prioridad 5
    xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 5, NULL);
}