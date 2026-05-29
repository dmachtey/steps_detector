#include "app_core.h"

// --- NUEVOS INCLUDES DEL HARDWARE DIVIDIDO ---
#include "hw_imu.h"
#include "hw_mic.h"
#include "gui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdlib.h> // Para abs()

// (Tus includes de WiFi y NVS que agregamos antes)
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <string.h>

#include "hw_rtc.h"

static const char *TAG = "APP_CORE";
static volatile estado_logger_t estado_actual = ESTADO_REPOSO;

// --- ESTRUCTURAS Y COLAS ---
typedef struct {
    int16_t x; int16_t y; int16_t z;
} imu_data_t;

static QueueHandle_t imu_queue = NULL;
static QueueHandle_t mic_queue = NULL; // Cola para la UI del micrófono

// =========================================================
// 1. TAREA IMU (50 Hz)
// =========================================================
static void imu_sampler_task(void *arg) {
    imu_data_t data;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    ESP_LOGI(TAG, "Tarea IMU iniciada (50 Hz)");
    while(1) {
        if (hw_imu_read(&data.x, &data.y, &data.z)) {
            xQueueSendToBack(imu_queue, &data, 0);
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// =========================================================
// 2. TAREA MICRÓFONO (Manejo de Buffers DMA 8 KHz)
// =========================================================
#define MIC_BUFFER_SAMPLES 400

static void mic_sampler_task(void *arg) {
    int16_t audio_buffer[MIC_BUFFER_SAMPLES];
    //    int loop_count = 0; // Contador para no saturar la terminal

    ESP_LOGI(TAG, "Tarea MIC iniciada (Bloques DMA de 8 KHz)");

    while(1) {
        if (hw_mic_read_dma(audio_buffer, MIC_BUFFER_SAMPLES)) {

            // Buscar el pico máximo (Envolvente de la onda)
            int16_t max_peak = 0;
            for (int i = 0; i < MIC_BUFFER_SAMPLES; i++) {
                int16_t val = abs(audio_buffer[i]);
                if (val > max_peak) {
                    max_peak = val;
                }
            }

            // Sonda: Imprimir el nivel del mic una vez por segundo (20 * 50ms)
            // if (loop_count++ % 20 == 0) {
            //     ESP_LOGI(TAG, "Nivel RMS/Pico MIC: %d", max_peak);
            // }

            xQueueSendToBack(mic_queue, &max_peak, 0);
        }
    }
}

// =========================================================
// 3. TAREA CONSUMIDORA: Refresco de Interfaz (GUI)
// =========================================================
static void telemetria_ui_task(void *arg) {
    imu_data_t rx_imu;
    int16_t rx_mic;

    ESP_LOGI(TAG, "Tarea Telemetría GUI iniciada");
    while(1) {
        // Usamos WHILE para vaciar absolutamente todo lo que haya en la cola
        while (xQueueReceive(imu_queue, &rx_imu, 0) == pdTRUE) {
            gui_update_chart_accel(rx_imu.x, rx_imu.y, rx_imu.z);
        }

        while (xQueueReceive(mic_queue, &rx_mic, 0) == pdTRUE) {
            gui_update_chart_mic(rx_mic);
        }

        // Dormimos 50ms para mantener el refresco visual a 20 FPS
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// =========================================================
// API DEL CORE
// =========================================================
void app_core_init(void) {
    ESP_LOGI(TAG, "Inicializando Core y colas...");
    estado_actual = ESTADO_REPOSO;

    imu_queue = xQueueCreate(20, sizeof(imu_data_t));
    mic_queue = xQueueCreate(20, sizeof(int16_t));

    app_core_wifi_init();
    // Tareas de muestreo ancladas al Core 1
    xTaskCreatePinnedToCore(imu_sampler_task, "IMU_TASK", 4096, NULL, 20, NULL, 1);
    xTaskCreatePinnedToCore(mic_sampler_task, "MIC_TASK", 4096, NULL, 21, NULL, 1);

    // Tarea GUI anclada al Core 0
    xTaskCreatePinnedToCore(telemetria_ui_task, "UI_TASK", 4096, NULL, 5, NULL, 0);
}

// =========================================================
// 4. LÓGICA DE WIFI Y MEMORIA NVS
// =========================================================
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi desconectado. Reintentando...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "¡WiFi Conectado! IP Asignada: " IPSTR,
                 IP2STR(&event->ip_info.ip));
        // Aquí el reloj NTP
        hw_rtc_sync_ntp();
    }
}

void app_core_guardar_wifi(const char* ssid, const char* pass) {
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
    nvs_set_str(my_handle, "ssid", ssid);
    nvs_set_str(my_handle, "pass", pass);
    nvs_commit(my_handle);
    nvs_close(my_handle);
    ESP_LOGI(TAG, "Credenciales WiFi guardadas en NVS permanentemente.");
}

void app_core_wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    // Leer la memoria NVS al arrancar
    nvs_handle_t my_handle;
    char ssid[32] = {0};
    char pass[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);

    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
        nvs_get_str(my_handle, "pass", pass, &pass_len);
        nvs_close(my_handle);
    }

    wifi_config_t wifi_config = {
        .sta = { .threshold = { .authmode = WIFI_AUTH_WPA2_PSK } },
    };

    if (strlen(ssid) > 0) {
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        ESP_LOGI(TAG, "Iniciando conexión a la red guardada: %s", ssid);
    } else {
        ESP_LOGI(TAG, "No hay credenciales WiFi en NVS. Modo espera.");
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}


void app_core_set_estado(estado_logger_t nuevo_estado) { estado_actual = nuevo_estado; }
estado_logger_t app_core_get_estado(void) { return estado_actual; }
void app_core_iniciar_grabacion(void) { app_core_set_estado(ESTADO_GRABANDO); }
void app_core_detener_grabacion(void) { app_core_set_estado(ESTADO_REPOSO); }
