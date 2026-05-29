#ifndef GUI_SCREENS_H
#define GUI_SCREENS_H

#include "lvgl.h"

// 1. Punteros Globales a las Pantallas (extern para que todos los archivos las vean)
extern lv_obj_t * scr_main;
extern lv_obj_t * scr_alarma;
extern lv_obj_t * scr_config;
extern lv_obj_t * scr_pasos;
extern lv_obj_t * scr_wifi;

// 2. Punteros Globales a las etiquetas del reloj
extern lv_obj_t * lbl_time_main;
extern lv_obj_t * lbl_time_alarma;
extern lv_obj_t * lbl_time_config;
extern lv_obj_t * lbl_time_pasos;
extern lv_obj_t * lbl_time_wifi;

// 3. Helpers comunes (Para no repetir código en cada pantalla)
lv_obj_t * crear_reloj_superior(lv_obj_t * parent);
void btn_volver_cb(lv_event_t * e);

// 4. Funciones constructoras de cada archivo
void gui_crear_pantalla_main(void);
void gui_crear_pantalla_alarma(void);
void gui_crear_pantalla_pasos(void);
void gui_crear_pantalla_config(void);
void gui_crear_pantalla_wifi(void);

#endif // GUI_SCREENS_H
