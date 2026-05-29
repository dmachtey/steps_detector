#include "gui_hostname.h"
#include "../app_core.h"

lv_obj_t * scr_hostname;
static lv_obj_t * ta_hostname = NULL;
static lv_obj_t * kb_hostname = NULL;

// Callback para manejar el cuadro de texto y el teclado
static void ta_hostname_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED) {
        if (kb_hostname) {
            lv_keyboard_set_textarea(kb_hostname, ta);
            lv_obj_clear_flag(kb_hostname, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (code == LV_EVENT_CANCEL) {
        if (kb_hostname) lv_obj_add_flag(kb_hostname, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
    }
    else if (code == LV_EVENT_READY) {
        const char * nuevo_nombre = lv_textarea_get_text(ta);
        app_core_set_device_name(nuevo_nombre);

        if (kb_hostname) lv_obj_add_flag(kb_hostname, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
    }
}

// Callback para el botón de volver
static void btn_volver_cb(lv_event_t * e) {
    extern lv_obj_t * scr_config;
    lv_scr_load(scr_config);
}

void gui_crear_pantalla_hostname(void) {
    scr_hostname = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_hostname, lv_color_black(), 0); // Fondo negro para mantener el estilo

    // Título de la pantalla
    lv_obj_t * lbl_titulo = lv_label_create(scr_hostname);
    lv_label_set_text(lbl_titulo, "Cambiar Nombre de Equipo");
    lv_obj_set_style_text_color(lbl_titulo, lv_color_white(), 0);
    lv_obj_align(lbl_titulo, LV_ALIGN_TOP_MID, 0, 20);

    // Cuadro de Texto
    ta_hostname = lv_textarea_create(scr_hostname);
    lv_textarea_set_one_line(ta_hostname, true);
    lv_textarea_set_max_length(ta_hostname, MAX_DEV_NAME_LEN - 1);
    lv_obj_set_width(ta_hostname, 250);
    lv_obj_align(ta_hostname, LV_ALIGN_TOP_MID, 0, 60);

    char dev_name[MAX_DEV_NAME_LEN];
    app_core_get_device_name(dev_name, sizeof(dev_name));
    lv_textarea_set_text(ta_hostname, dev_name);

    lv_obj_add_event_cb(ta_hostname, ta_hostname_event_cb, LV_EVENT_ALL, NULL);

    // --- EL BOTÓN CORREGIDO (Gris, Abajo a la Derecha) ---
    lv_obj_t * btn_volver = lv_btn_create(scr_hostname);
    lv_obj_set_size(btn_volver, 140, 50);
    lv_obj_align(btn_volver, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(btn_volver, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_volver, btn_volver_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lbl_volver = lv_label_create(btn_volver);
    lv_label_set_text(lbl_volver, "Volver");
    lv_obj_center(lbl_volver);

    // Teclado Virtual
    kb_hostname = lv_keyboard_create(scr_hostname);
    lv_obj_add_flag(kb_hostname, LV_OBJ_FLAG_HIDDEN);
}
