#include "gui_screens.h"

void gui_crear_pantalla_alarma(void) {
    scr_alarma = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_alarma, lv_palette_darken(LV_PALETTE_RED, 3), 0);
    lbl_time_alarma = crear_reloj_superior(scr_alarma);

    lv_obj_t * btn_detener = lv_btn_create(scr_alarma);
    lv_obj_set_size(btn_detener, 200, 60);
    lv_obj_align(btn_detener, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn_detener, btn_volver_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * lbl_detener = lv_label_create(btn_detener);
    lv_label_set_text(lbl_detener, "Desactivar");
    lv_obj_center(lbl_detener);
}