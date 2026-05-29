#include "gui_screens.h"

// Callbacks específicos de navegación desde el Main
static void btn_ir_alarma_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_scr_load(scr_alarma);
}

static void btn_ir_config_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_scr_load(scr_config);
}

void gui_crear_pantalla_main(void) {
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, lv_color_black(), 0);
    
    // Usamos el helper común para el reloj
    lbl_time_main = crear_reloj_superior(scr_main);

    // Botón Alarma
    lv_obj_t * btn_alarma = lv_btn_create(scr_main);
    lv_obj_set_size(btn_alarma, 240, 80);
    lv_obj_align(btn_alarma, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_bg_color(btn_alarma, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_alarma, btn_ir_alarma_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * lbl_alarma = lv_label_create(btn_alarma);
    lv_label_set_text(lbl_alarma, "Activar Alarma");
    lv_obj_center(lbl_alarma);

    // Botón Configuración
    lv_obj_t * btn_config = lv_btn_create(scr_main);
    lv_obj_set_size(btn_config, 240, 80);
    lv_obj_align(btn_config, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(btn_config, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_config, btn_ir_config_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * lbl_config = lv_label_create(btn_config);
    lv_label_set_text(lbl_config, "Configuracion");
    lv_obj_center(lbl_config);
}