#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stdbool.h>

// Crear el árbol de pantallas
void gui_crear_pantallas(void);

// API Pública para actualizar datos en pantalla
void gui_set_time(int hora, int minuto);
void gui_update_chart_accel(int16_t x, int16_t y, int16_t z);
void gui_update_chart_mic(int16_t mic_val);

#endif // GUI_H
