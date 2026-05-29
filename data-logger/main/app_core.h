#ifndef APP_CORE_H
#define APP_CORE_H

#include <stdbool.h>

typedef enum {
    ESTADO_REPOSO,
    ESTADO_GRABANDO
} estado_logger_t;

void app_core_init(void);
void app_core_set_estado(estado_logger_t nuevo_estado);
estado_logger_t app_core_get_estado(void);

// Lógica simulada por ahora
void app_core_iniciar_grabacion(void);
void app_core_detener_grabacion(void);

// Funciones de WiFi y NVS
void app_core_wifi_init(void);
void app_core_guardar_wifi(const char *ssid, const char *pass);

#endif
