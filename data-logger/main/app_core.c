#include "app_core.h"
#include "hardware/hw_imu.h"
#include "hardware/hw_mic.h"
#include "hardware/hw_rtc.h"
#include "gui/gui.h"
#include "logger.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// --- ESTAS LIBRERÍAS FALTABAN ---
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
// --------------------------------
#include "esp_system.h"


static const char *TAG = "APP_CORE";
static volatile estado_logger_t estado_actual = ESTADO_REPOSO;
static volatile bool ui_telemetria_activa = false;

typedef struct { int16_t x; int16_t y; int16_t z; } imu_data_t;

static QueueHandle_t imu_queue = NULL;
static QueueHandle_t mic_queue = NULL;

// =========================================================
// 1. TAREA IMU (50 Hz)
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
        }
    }
}

// =========================================================
// 3. TAREA GUI Y API DEL CORE
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

void app_core_init(void) {
    ESP_LOGI(TAG, "Inicializando Core...");
    estado_actual = ESTADO_REPOSO;
    ui_telemetria_activa = false;

    logger_init();

    imu_queue = xQueueCreate(20, sizeof(imu_data_t));
    mic_queue = xQueueCreate(20, sizeof(int16_t));

    app_core_wifi_init();

    xTaskCreatePinnedToCore(imu_sampler_task, "IMU_TASK", 4096, NULL, 20, NULL, 1);
    xTaskCreatePinnedToCore(mic_sampler_task, "MIC_TASK", 4096, NULL, 21, NULL, 1);
    xTaskCreatePinnedToCore(telemetria_ui_task, "UI_TASK", 4096, NULL, 5, NULL, 0);
    ESP_LOGW(TAG, ">>> RAM Libre actual: %ld bytes <<<", (long)esp_get_free_heap_size());
}

// --- CONTROLES DE LA UI ---
void app_core_set_telemetria_activa(bool activa) {
    ui_telemetria_activa = activa;
}

void app_core_set_estado(estado_logger_t nuevo_estado) {
    estado_actual = nuevo_estado;
}

estado_logger_t app_core_get_estado(void) {
    return estado_actual;
}

void app_core_iniciar_grabacion_parametros(const char* actividad, int demora_segundos) {
    app_core_set_estado(ESTADO_GRABANDO);
    logger_start(actividad, demora_segundos);
}

void app_core_detener_grabacion_y_recortar(void) {
    logger_stop();
    app_core_set_estado(ESTADO_REPOSO);
}

void app_core_iniciar_grabacion(void) {
    app_core_set_estado(ESTADO_GRABANDO);
}

void app_core_detener_grabacion(void) {
    app_core_set_estado(ESTADO_REPOSO);
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
        ESP_LOGI(TAG, "¡WiFi Conectado! IP Asignada: " IPSTR, IP2STR(&event->ip_info.ip));
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
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

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

    char dev_name[MAX_DEV_NAME_LEN];
    app_core_get_device_name(dev_name, sizeof(dev_name));
    esp_netif_set_hostname(sta_netif, dev_name);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_core_set_device_name(const char* name) {
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_str(my_handle, "dev_name", name);
        nvs_commit(my_handle);
        nvs_close(my_handle);

        ESP_LOGI(TAG, "Nombre de equipo guardado en NVS: %s", name);

        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif) {
            esp_netif_set_hostname(netif, name);
        }
    }
}

void app_core_get_device_name(char* out_name, size_t max_len) {
    nvs_handle_t my_handle;
    strncpy(out_name, "Datalogger_ESP", max_len);

    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) {
        size_t required_size = max_len;
        if (nvs_get_str(my_handle, "dev_name", out_name, &required_size) == ESP_OK) {
            ESP_LOGI(TAG, "Nombre de equipo cargado: %s", out_name);
        }
        nvs_close(my_handle);
    }
}
