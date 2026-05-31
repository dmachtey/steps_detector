#include "alarm_core.h"
#include "dsp_audio.h"
#include "Classifier.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h" // <-- IMPORTANTE

static const char *TAG = "ALARM_CORE";

// Features es chico (1.8 KB), puede quedar en la RAM interna
static float features[453];

// El buffer gigante lo pasamos a puntero
static float *audio_buffer = NULL;

static void alarm_task(void *pvParameters) {
    ESP_LOGI(TAG, "Tarea de vigilancia iniciada.");

    while (1) {
        // ... (Tu código de recolección, extracción e inferencia queda igual) ...
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void alarm_core_start_task(void) {
    // Pedir memoria para el buffer de audio en la PSRAM
    audio_buffer = (float *)heap_caps_malloc(24000 * sizeof(float), MALLOC_CAP_SPIRAM);

    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Error: No hay PSRAM para el buffer de audio.");
        return;
    }

    dsp_audio_init();
    xTaskCreate(alarm_task, "alarm_task", 4096, NULL, 5, NULL);
}
