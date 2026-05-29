#include "gui_screens.h"
#include "app_core.h"
#include "esp_log.h"

static const char *TAG = "GUI_WIFI";

static lv_obj_t * ta_ssid;
static lv_obj_t * ta_pass;
static lv_obj_t * kb_wifi;

static void btn_volver_config_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_scr_load(scr_config);
}

// Muestra u oculta el teclado dependiendo de si tocas el cuadro de texto
static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb_wifi, ta);
        lv_obj_clear_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
    }
    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb_wifi, NULL);
        lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
    }
}

static void btn_guardar_wifi_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        const char * ssid = lv_textarea_get_text(ta_ssid);
        const char * pass = lv_textarea_get_text(ta_pass);
        
        ESP_LOGI(TAG, "Guardando credenciales WiFi...");
        app_core_guardar_wifi(ssid, pass);
        
        lv_scr_load(scr_config);
    }
}

void gui_crear_pantalla_wifi(void) {
    scr_wifi = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_wifi, lv_color_black(), 0);
    lbl_time_wifi = crear_reloj_superior(scr_wifi);

    lv_obj_t * lbl_tit_wifi = lv_label_create(scr_wifi);
    lv_label_set_text(lbl_tit_wifi, "Configurar WiFi");
    lv_obj_set_style_text_color(lbl_tit_wifi, lv_color_white(), 0);
    lv_obj_align(lbl_tit_wifi, LV_ALIGN_TOP_LEFT, 20, 15);

    ta_ssid = lv_textarea_create(scr_wifi);
    lv_textarea_set_one_line(ta_ssid, true);
    lv_textarea_set_placeholder_text(ta_ssid, "Nombre de la Red");
    lv_obj_set_width(ta_ssid, 300);
    lv_obj_align(ta_ssid, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_event_cb(ta_ssid, ta_event_cb, LV_EVENT_ALL, NULL);

    ta_pass = lv_textarea_create(scr_wifi);
    lv_textarea_set_one_line(ta_pass, true);
    lv_textarea_set_password_mode(ta_pass, true);
    lv_textarea_set_placeholder_text(ta_pass, "Contrasena");
    lv_obj_set_width(ta_pass, 300);
    lv_obj_align(ta_pass, LV_ALIGN_TOP_MID, 0, 110);
    lv_obj_add_event_cb(ta_pass, ta_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * btn_guardar_wifi = lv_btn_create(scr_wifi);
    lv_obj_set_size(btn_guardar_wifi, 140, 50);
    lv_obj_align(btn_guardar_wifi, LV_ALIGN_TOP_RIGHT, -30, 160);
    lv_obj_set_style_bg_color(btn_guardar_wifi, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_guardar_wifi, btn_guardar_wifi_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * lbl_g_wifi = lv_label_create(btn_guardar_wifi);
    lv_label_set_text(lbl_g_wifi, "Guardar");
    lv_obj_center(lbl_g_wifi);

    lv_obj_t * btn_cancelar_wifi = lv_btn_create(scr_wifi);
    lv_obj_set_size(btn_cancelar_wifi, 140, 50);
    lv_obj_align(btn_cancelar_wifi, LV_ALIGN_TOP_LEFT, 30, 160);
    lv_obj_set_style_bg_color(btn_cancelar_wifi, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_cancelar_wifi, btn_volver_config_cb, LV_EVENT_ALL, NULL);
    lv_obj_t * lbl_c_wifi = lv_label_create(btn_cancelar_wifi);
    lv_label_set_text(lbl_c_wifi, "Cancelar");
    lv_obj_center(lbl_c_wifi);

    kb_wifi = lv_keyboard_create(scr_wifi);
    lv_obj_align(kb_wifi, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb_wifi, LV_OBJ_FLAG_HIDDEN);
}