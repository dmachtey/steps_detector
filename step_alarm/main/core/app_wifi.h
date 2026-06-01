#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DEV_NAME_LEN 32

// Inicializa el WiFi (modo Station), lee credenciales de NVS y conecta.
void app_wifi_init(void);

// Guarda las credenciales WiFi en la memoria no volátil (NVS).
void app_wifi_guardar_credenciales(const char* ssid, const char* pass);

// Establece y guarda el nombre de red (hostname) del dispositivo.
void app_wifi_set_device_name(const char* name);

// Lee el nombre del dispositivo desde NVS.
void app_wifi_get_device_name(char* out_name, size_t max_len);

#ifdef __cplusplus
}
#endif