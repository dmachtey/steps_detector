#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Estados del sistema / datalogger
typedef enum {
    ESTADO_REPOSO,
    ESTADO_GRABANDO,
    ESTADO_ERROR
} estado_logger_t;

// Inicializa las colas, logger y lanza tareas base
void app_core_init(void);

// Controles de interfaz gráfica
void app_core_set_telemetria_activa(bool activa);

// Controles de estados y logger
void app_core_set_estado(estado_logger_t nuevo_estado);
estado_logger_t app_core_get_estado(void);
void app_core_iniciar_grabacion_parametros(const char* actividad, int demora_segundos);
void app_core_detener_grabacion_y_recortar(void);
void app_core_iniciar_grabacion(void);
void app_core_detener_grabacion(void);

#ifdef __cplusplus
}
#endif