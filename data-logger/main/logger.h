#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Inicializa el sistema de logueo
void logger_init(void);

// Inicia la secuencia (crea archivos y maneja el retardo)
void logger_start(const char* actividad, int demora_segundos);

// Detiene la grabación y recorta los últimos 3 segundos
void logger_stop(void);

// Funciones para inyectar datos desde las tareas de app_core
void logger_feed_imu(int16_t x, int16_t y, int16_t z);
void logger_feed_mic(int16_t *buffer, size_t num_samples);

// Permite saber al core si el logger realmente está grabando (pasó el retardo)
bool logger_is_recording(void);

#endif // LOGGER_H