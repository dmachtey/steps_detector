#include "hw_rtc.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "HW_RTC";

// Callback que el sistema operativo llama automáticamente cuando logra sincronizar la hora
static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "¡Sincronización NTP exitosa!");
}

void hw_rtc_init(void) {
    ESP_LOGI(TAG, "Inicializando RTC Interno...");
    
    // Configuramos UTC-3 (Argentina) por defecto
    hw_rtc_set_timezone("ART3"); 
}

void hw_rtc_set_timezone(const char *tz) {
    ESP_LOGI(TAG, "Configurando Zona Horaria a: %s", tz);
    // setenv modifica la variable de entorno TZ del sistema POSIX interno
    setenv("TZ", tz, 1);
    tzset(); // Aplica los cambios inmediatamente
}

void hw_rtc_sync_ntp(void) {
    ESP_LOGI(TAG, "Iniciando servicio SNTP...");
    
    // Si ya estaba corriendo (ej. nos desconectamos y reconectamos), lo reiniciamos
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    
    esp_sntp_init();
}

bool hw_rtc_get_time(struct tm *timeinfo) {
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    
    // El ESP32 arranca su reloj en 1970 (año 70 para struct tm). 
    // Si el año es mayor a 2024 (124 desde 1900), asumimos que el NTP ya actualizó la hora.
    if (timeinfo->tm_year >= 124) {
        return true;
    }
    return false;
}