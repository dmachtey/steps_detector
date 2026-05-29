#include "gui_screens.h"

// Callbacks específicos de navegación interna
static void btn_ir_pasos_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_scr_load(scr_pasos);
}

static void btn_ir_wifi_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_scr_load(scr_wifi);
}

void gui_crear_pantalla_config(void) {
    scr_config = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_config, lv_color_black(), 0);
    lbl_time_config = crear_reloj_superior(scr_config);

    lv_obj_t * lbl_tit_conf = lv_label_create(scr_config);
    lv_label_set_text(lbl_tit_conf, "Ajustes del Sistema");
    lv_obj_set_style_text_color(lbl_tit_conf, lv_color_white(), 0);
    lv_obj_align(lbl_tit_conf, LV_ALIGN_TOP_LEFT, 20, 15);

    lv_obj_t * list_conf = lv_list_create(scr_config);
    lv_obj_set_size(list_conf, 320, 280);
    lv_obj_align(list_conf, LV_ALIGN_CENTER, 0, 10);

    lv_list_add_text(list_conf, "Aplicaciones");
    lv_obj_t * list_btn_pasos = lv_list_add_btn(list_conf, LV_SYMBOL_PLAY, " Registro de Sensores");
    lv_obj_add_event_cb(list_btn_pasos, btn_ir_pasos_cb, LV_EVENT_ALL, NULL);

    lv_list_add_text(list_conf, "Conectividad");
    lv_obj_t * list_btn_wifi = lv_list_add_btn(list_conf, LV_SYMBOL_WIFI, " WiFi Setup");
    lv_obj_add_event_cb(list_btn_wifi, btn_ir_wifi_cb, LV_EVENT_ALL, NULL);
    lv_list_add_btn(list_conf, LV_SYMBOL_SETTINGS, " Servidor MQTT");

    lv_list_add_text(list_conf, "Dispositivo");
    lv_list_add_btn(list_conf, LV_SYMBOL_EDIT, " Nombre Equipo");
    lv_list_add_btn(list_conf, LV_SYMBOL_BELL, " Sensores / Hardware");
    lv_list_add_btn(list_conf, LV_SYMBOL_FILE, " Reloj (RTC)");

    lv_obj_t * btn_volverc = lv_btn_create(scr_config);
    lv_obj_set_size(btn_volverc, 160, 50);
    lv_obj_align(btn_volverc, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_volverc, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_volverc, btn_volver_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * lbl_volverc = lv_label_create(btn_volverc);
    lv_label_set_text(lbl_volverc, "Volver");
    lv_obj_center(lbl_volverc);
}