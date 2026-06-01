#include "app_wifi.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#include "hardware/hw_rtc.h" // Para sincronizar la hora al conectar

static const char *TAG = "APP_WIFI";

// Manejador de eventos de red
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { 
        esp_wifi_connect(); 
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { 
        ESP_LOGW(TAG, "WiFi desconectado. Reintentando..."); 
        esp_wifi_connect(); 
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { 
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; 
        ESP_LOGI(TAG, "¡WiFi Conectado! IP Asignada: " IPSTR, IP2STR(&event->ip_info.ip)); 
        
        // Sincronizamos el reloj NTP al tener internet
        hw_rtc_sync_ntp(); 
    }
}

void app_wifi_init(void) {
    ESP_LOGI(TAG, "Inicializando módulo WiFi...");
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
        .sta = { 
            .threshold = { .authmode = WIFI_AUTH_WPA2_PSK } 
        }, 
    }; 
    
    if (strlen(ssid) > 0) { 
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid)); 
        strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password)); 
        ESP_LOGI(TAG, "Conectando a la red guardada: %s", ssid); 
    } else { 
        ESP_LOGI(TAG, "No hay credenciales WiFi en NVS. Modo espera."); 
    } 
    
    char dev_name[MAX_DEV_NAME_LEN]; 
    app_wifi_get_device_name(dev_name, sizeof(dev_name)); 
    esp_netif_set_hostname(sta_netif, dev_name); 
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); 
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); 
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_wifi_guardar_credenciales(const char* ssid, const char* pass) {
    nvs_handle_t my_handle; 
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle)); 
    nvs_set_str(my_handle, "ssid", ssid); 
    nvs_set_str(my_handle, "pass", pass); 
    nvs_commit(my_handle); 
    nvs_close(my_handle); 
    ESP_LOGI(TAG, "Credenciales WiFi guardadas en NVS.");
}

void app_wifi_set_device_name(const char* name) {
    nvs_handle_t my_handle; 
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) { 
        nvs_set_str(my_handle, "dev_name", name); 
        nvs_commit(my_handle); 
        nvs_close(my_handle); 
        ESP_LOGI(TAG, "Nombre guardado en NVS: %s", name); 
        
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"); 
        if (netif) { 
            esp_netif_set_hostname(netif, name); 
        } 
    }
}

void app_wifi_get_device_name(char* out_name, size_t max_len) {
    nvs_handle_t my_handle; 
    strncpy(out_name, "Datalogger_ESP", max_len); 
    if (nvs_open("storage", NVS_READONLY, &my_handle) == ESP_OK) { 
        size_t required_size = max_len; 
        nvs_get_str(my_handle, "dev_name", out_name, &required_size); 
        nvs_close(my_handle); 
    }
}