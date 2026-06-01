#include "app_core.h"
#include "app_wifi.h"         // La nueva capa de red
#include "config.h"           // Variables maestras y flags
#include "hardware/hw_imu.h"
#include "hardware/hw_mic.h"
#include "services/logger.h"  // Asumiendo que moviste logger a services/
#include "gui/gui.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdlib.h>

static const char *TAG = "APP_CORE";

static volatile estado_logger_t estado_actual = ESTADO_REPOSO;
static volatile bool ui_telemetria_activa = false;

// Estructura local
typedef struct { int16_t x; int16_t y; int16_t z; } imu_data_t;

// Colas privadas para la UI
static QueueHandle_t imu_queue = NULL;
static QueueHandle_t mic_queue = NULL;

// Colas públicas para el IA (nacen acá, declaradas extern en config.h)
QueueHandle_t alarm_imu_queue = NULL;
QueueHandle_t alarm_mic_queue = NULL;

#define MIC_BUFFER_SAMPLES 160

// =========================================================
// 1. TAREAS DE SENSORES (MUESTREO FÍSICO)
// =========================================================
static void imu_sampler_task(void *arg) {
    imu_data_t data;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    while(1) {
        if (hw_imu_read(&data.x, &data.y, &data.z)) {
            if (ui_telemetria_activa) {
                xQueueSendToBack(imu_queue, &data, 0);
            }
            if (estado_actual == ESTADO_GRABANDO) {
                logger_feed_imu(data.x, data.y, data.z);
            }
            if (alarm_imu_queue != NULL) {
                xQueueSendToBack(alarm_imu_queue, &data, 0);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

static void mic_sampler_task(void *arg) {
    int16_t audio_buffer[MIC_BUFFER_SAMPLES];

    while(1) {
        if (hw_mic_read_dma(audio_buffer, MIC_BUFFER_SAMPLES)) {
            if (ui_telemetria_activa) {
                int16_t max_peak = 0;
                for (int i = 0; i < MIC_BUFFER_SAMPLES; i++) {
                    int16_t val = abs(audio_buffer[i]);
                    if (val > max_peak) max_peak = val;
                }
                xQueueSendToBack(mic_queue, &max_peak, 0);
            }
            if (estado_actual == ESTADO_GRABANDO) {
                logger_feed_mic(audio_buffer, MIC_BUFFER_SAMPLES);
            }
            if (alarm_mic_queue != NULL) {
                xQueueSendToBack(alarm_mic_queue, audio_buffer, 0);
            }
        }
    }
}

// =========================================================
// 2. TAREA UI DE TELEMETRÍA
// =========================================================
static void telemetria_ui_task(void *arg) {
    imu_data_t rx_imu;
    int16_t rx_mic;

    while(1) {
        if (ui_telemetria_activa) {
            while (xQueueReceive(imu_queue, &rx_imu, 0) == pdTRUE) {
                gui_update_chart_accel(rx_imu.x, rx_imu.y, rx_imu.z);
            }
            while (xQueueReceive(mic_queue, &rx_mic, 0) == pdTRUE) {
                gui_update_chart_mic(rx_mic);
            }
        } else {
            xQueueReset(imu_queue);
            xQueueReset(mic_queue);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// =========================================================
// 3. INICIALIZACIÓN DEL CORE
// =========================================================
void app_core_init(void) {
    ESP_LOGI(TAG, "Inicializando Core del Sistema...");
    estado_actual = ESTADO_REPOSO;
    ui_telemetria_activa = false;

    logger_init();

    imu_queue = xQueueCreate(20, sizeof(imu_data_t));
    mic_queue = xQueueCreate(20, sizeof(int16_t));
    alarm_imu_queue = xQueueCreate(10, sizeof(imu_data_t));
    alarm_mic_queue = xQueueCreate(10, MIC_BUFFER_SAMPLES * sizeof(int16_t));

    xTaskCreatePinnedToCore(telemetria_ui_task, "UI_TASK", 4096, NULL, 5, NULL, 0);

#if MODO_SIMULADOR_SD == 0
    ESP_LOGI(TAG, "Hardware Real Activado. Levantando sensores y WiFi...");
    app_wifi_init();
    xTaskCreatePinnedToCore(imu_sampler_task, "IMU_TASK", 4096, NULL, 20, NULL, 1);
    xTaskCreatePinnedToCore(mic_sampler_task, "MIC_TASK", 4096, NULL, 21, NULL, 1);
#else
    ESP_LOGW(TAG, "⚠️ MODO SIMULADOR ACTIVO: Sensores y WiFi físicos apagados. ⚠️");
#endif

    ESP_LOGW(TAG, ">>> RAM Libre post-Core: %ld bytes <<<", (long)esp_get_free_heap_size());
}

// =========================================================
// 4. MÉTODOS DE CONTROL (API)
// =========================================================
void app_core_set_telemetria_activa(bool activa) { ui_telemetria_activa = activa; }
void app_core_set_estado(estado_logger_t nuevo_estado) { estado_actual = nuevo_estado; }
estado_logger_t app_core_get_estado(void) { return estado_actual; }
void app_core_iniciar_grabacion_parametros(const char* actividad, int demora_segundos) { 
    app_core_set_estado(ESTADO_GRABANDO); 
    logger_start(actividad, demora_segundos); 
}
void app_core_detener_grabacion_y_recortar(void) { 
    logger_stop(); 
    app_core_set_estado(ESTADO_REPOSO); 
}
void app_core_iniciar_grabacion(void) { app_core_set_estado(ESTADO_GRABANDO); }
void app_core_detener_grabacion(void) { app_core_set_estado(ESTADO_REPOSO); }