#ifndef GUI_HOSTNAME_H
#define GUI_HOSTNAME_H

#include "lvgl.h"

// Exponemos la pantalla para poder cargarla desde otros menús
extern lv_obj_t * scr_hostname;

// Crea los objetos de la pantalla (se llama una sola vez al inicio)
void gui_crear_pantalla_hostname(void);

#endif // GUI_HOSTNAME_H