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

// --- LIBRERÍAS DEL SISTEMA ---
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_system.h"

// 🚩 INCLUIMOS LA CONFIGURACIÓN MAESTRA
#include "config.h"

static const char *TAG = "APP_CORE";
static volatile estado_logger_t estado_actual = ESTADO_REPOSO;
static volatile bool ui_telemetria_activa = false;

// Estructura de datos del acelerómetro
typedef struct { int16_t x; int16_t y; int16_t z; } imu_data_t;

// Colas privadas para la UI
static QueueHandle_t imu_queue = NULL;
static QueueHandle_t mic_queue = NULL;

// --- COLAS PÚBLICAS PARA LA ALARMA (Productor-Consumidor) ---
// Ahora se definen acá (nacen acá), pero se declaran extern en config.h
QueueHandle_t alarm_imu_queue = NULL;
QueueHandle_t alarm_mic_queue = NULL;

// =========================================================
// 1. TAREA IMU (50 Hz)
// =========================================================
static void imu_sampler_task(void *arg) {
    imu_data_t data;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);

    while(1) {
        if (hw_imu_read(&data.x, &data.y, &data.z)) {
            // Reparto 1: UI
            if (ui_telemetria_activa) {
                xQueueSendToBack(imu_queue, &data, 0);
            }
            // Reparto 2: Datalogger (Graba el dato crudo en SD)
            if (estado_actual == ESTADO_GRABANDO) {
                logger_feed_imu(data.x, data.y, data.z);
            }
            // Reparto 3: IA / Alarma (Manda crudo por la cola)
            if (alarm_imu_queue != NULL) {
                xQueueSendToBack(alarm_imu_queue, &data, 0);
            }
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// =========================================================
// 2. TAREA MICRÓFONO (Manejo de Buffers DMA 8 KHz)
// =========================================================
#define MIC_BUFFER_SAMPLES 160

static void mic_sampler_task(void *arg) {
    int16_t audio_buffer[MIC_BUFFER_SAMPLES];

    while(1) {
        if (hw_mic_read_dma(audio_buffer, MIC_BUFFER_SAMPLES)) {
            // Reparto 1: UI (Calcula pico máximo)
            if (ui_telemetria_activa) {
                int16_t max_peak = 0;
                for (int i = 0; i < MIC_BUFFER_SAMPLES; i++) {
                    int16_t val = abs(audio_buffer[i]);
                    if (val > max_peak) max_peak = val;
                }
                xQueueSendToBack(mic_queue, &max_peak, 0);
            }

            // Reparto 2: Datalogger (Graba chunk en SD)
            if (estado_actual == ESTADO_GRABANDO) {
                logger_feed_mic(audio_buffer, MIC_BUFFER_SAMPLES);
            }

            // Reparto 3: IA / Alarma (Manda chunk por la cola)
            if (alarm_mic_queue != NULL) {
                xQueueSendToBack(alarm_mic_queue, audio_buffer, 0);
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
            // Limpia búferes si la gráfica está apagada para no atrasarse
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

    // --- MONTAJE DE LA SD Y LOGGER ---
    // (Esto debe arrancar SIEMPRE, ya sea para grabar en producción o leer en simulador)
    logger_init();

    // --- CREACIÓN DE COLAS DE MEMORIA ---
    // Colas Privadas UI
    imu_queue = xQueueCreate(20, sizeof(imu_data_t));
    mic_queue = xQueueCreate(20, sizeof(int16_t));

    // Colas Públicas IA
    alarm_imu_queue = xQueueCreate(10, sizeof(imu_data_t));
    alarm_mic_queue = xQueueCreate(10, MIC_BUFFER_SAMPLES * sizeof(int16_t));
#if MODO_SIMULADOR_SD == 0
    // MODO PRODUCCIÓN
    // Iniciar WiFi
    app_core_wifi_init();
#endif
    // Siempre lanzamos la UI
    xTaskCreatePinnedToCore(telemetria_ui_task, "UI_TASK", 4096, NULL, 5, NULL, 0);

// =========================================================
// 🚩 BYPASS DE SENSORES: EL ENGAÑO MAESTRO
// =========================================================
#if MODO_SIMULADOR_SD == 0
    // MODO PRODUCCIÓN: Levantamos los sensores reales
    ESP_LOGI(TAG, "Hardware Real Activado. Iniciando muestreo físico...");
    xTaskCreatePinnedToCore(imu_sampler_task, "IMU_TASK", 4096, NULL, 20, NULL, 1);
    xTaskCreatePinnedToCore(mic_sampler_task, "MIC_TASK", 4096, NULL, 21, NULL, 1);
#else
    // MODO SIMULADOR: Apagamos los sensores reales.
    // El archivo mock_sensors.c se encargará de inyectar datos en las colas.
    ESP_LOGW(TAG, "⚠️ MODO SIMULADOR ACTIVO: Sensores I2C/I2S deshabilitados. ⚠️");
#endif

    ESP_LOGW(TAG, ">>> RAM Libre actual: %ld bytes <<<", (long)esp_get_free_heap_size());
}

// ... (Acá sigue todo el resto de tus funciones de UI y WiFi sin cambios)
void app_core_set_telemetria_activa(bool activa) { ui_telemetria_activa = activa; }
void app_core_set_estado(estado_logger_t nuevo_estado) { estado_actual = nuevo_estado; }
estado_logger_t app_core_get_estado(void) { return estado_actual; }
void app_core_iniciar_grabacion_parametros(const char* actividad, int demora_segundos) { app_core_set_estado(ESTADO_GRABANDO); logger_start(actividad, demora_segundos); }
void app_core_detener_grabacion_y_recortar(void) { logger_stop(); app_core_set_estado(ESTADO_REPOSO); }
void app_core_iniciar_grabacion(void) { app_core_set_estado(ESTADO_GRABANDO); }
void app_core_detener_grabacion(void) { app_core_set_estado(ESTADO_REPOSO); }

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { esp_wifi_connect(); }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { ESP_LOGW(TAG, "WiFi desconectado. Reintentando..."); esp_wifi_connect(); }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; ESP_LOGI(TAG, "¡WiFi Conectado! IP Asignada: " IPSTR, IP2STR(&event->ip_info.ip)); hw_rtc_sync_ntp(); }
}
void app_core_guardar_wifi(const char* ssid, const char* pass) { nvs_handle_t my_handle; ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle)); nvs_set_str(my_handle, "ssid", ssid); nvs_set_str(my_handle, "pass", pass); nvs_commit(my_handle); nvs_close(my_handle); ESP_LOGI(TAG, "Credenciales WiFi guardadas en NVS permanentemente."); }
void app_core_wifi_init(void) { ESP_ERROR_CHECK(esp_netif_init()); ESP_ERROR_CHECK(esp_event_loop_create_default()); esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta(); wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); ESP_ERROR_CHECK(esp_wifi_init(&cfg)); esp_event_handler_instance_t instance_any_id; esp_event_handler_instance_t instance_got_ip; ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id)); ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip)); nvs_handle_t my_handle; char ssid[32] = {0}; char pass[64] = {0}; size_t ssid_len = sizeof(ssid); size_t pass_len = sizeof(pass); if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) { nvs_get_str(my_handle, "ssid", ssid, &ssid_len); nvs_get_str(my_handle, "pass", pass, &pass_len); nvs_close(my_handle); } wifi_config_t wifi_config = { .sta = { .threshold = { .authmode = WIFI_AUTH_WPA2_PSK } }, }; if (strlen(ssid) > 0) { strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)); strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password)); ESP_LOGI(TAG, "Iniciando conexión a la red guardada: %s", ssid); } else { ESP_LOGI(TAG, "No hay credenciales WiFi en NVS. Modo espera."); } char dev_name[MAX_DEV_NAME_LEN]; app_core_get_device_name(dev_name, sizeof(dev_name)); esp_netif_set_hostname(sta_netif, dev_name); ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); ESP_ERROR_CHECK(esp_wifi_start()); }
void app_core_set_device_name(const char* name) { nvs_handle_t my_handle; if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) { nvs_set_str(my_handle, "dev_name", name); nvs_commit(my_handle); nvs_close(my_handle); ESP_LOGI(TAG, "Nombre de equipo guardado en NVS: %s", name); esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"); if (netif) { esp_netif_set_hostname(netif, name); } } }
void app_core_get_device_name(char* out_name, size_t max_len) { nvs_handle_t my_handle; strncpy(out_name, "Datalogger_ESP", max_len); if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) { size_t required_size = max_len; if (nvs_get_str(my_handle, "dev_name", out_name, &required_size) == ESP_OK) { ESP_LOGI(TAG, "Nombre de equipo cargado: %s", out_name); } nvs_close(my_handle); } }
