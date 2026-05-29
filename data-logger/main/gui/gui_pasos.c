#include "gui_screens.h"
#include "gui_hw.h" // Para usar gui_lock()
#include "../app_core.h"
#include <stdlib.h> // Para atoi()

// Variables de los gráficos exclusivas de esta pantalla
static lv_obj_t * chart_accel;
static lv_chart_series_t * ser_x;
static lv_chart_series_t * ser_y;
static lv_chart_series_t * ser_z;
static lv_obj_t * chart_mic;
static lv_chart_series_t * ser_mic;

// Variables de los controles superiores
static lv_obj_t * dd_actividad;
static lv_obj_t * dd_demora;
static lv_obj_t * btn_accion;
static lv_obj_t * lbl_accion;

// ==========================================
// API DE ACTUALIZACIÓN DE GRÁFICOS
// ==========================================
void gui_update_chart_accel(int16_t x, int16_t y, int16_t z) {
    if (gui_lock(10)) {
        if (chart_accel != NULL && lv_scr_act() == scr_pasos) {
            lv_chart_set_next_value(chart_accel, ser_x, x);
            lv_chart_set_next_value(chart_accel, ser_y, y);
            lv_chart_set_next_value(chart_accel, ser_z, z);
        }
        gui_unlock();
    }
}

void gui_update_chart_mic(int16_t mic_val) {
    if (gui_lock(10)) {
        if (chart_mic != NULL && lv_scr_act() == scr_pasos) {
            lv_chart_set_next_value(chart_mic, ser_mic, mic_val);
        }
        gui_unlock();
    }
}

// ==========================================
// CALLBACK DEL BOTÓN INICIAR/DETENER
// ==========================================
static void btn_accion_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        
        if (app_core_get_estado() == ESTADO_REPOSO) {
            // 1. Extraemos los parámetros de los combos
            char actividad[32];
            char demora_str[10];
            lv_dropdown_get_selected_str(dd_actividad, actividad, sizeof(actividad));
            lv_dropdown_get_selected_str(dd_demora, demora_str, sizeof(demora_str));
            int demora_segundos = atoi(demora_str); // "5s" se convierte mágicamente en 5

            // 2. Cambiamos la estética del botón a rojo
            lv_label_set_text(lbl_accion, "Detener");
            lv_obj_set_style_bg_color(btn_accion, lv_palette_main(LV_PALETTE_RED), 0);
            
            // PREPARADO PARA LA LÓGICA: Acá en el próximo paso llamaremos a:
            // app_core_iniciar_grabacion_parametros(actividad, demora_segundos);
            app_core_set_estado(ESTADO_GRABANDO); // Simulamos el cambio por ahora

        } else {
            // 1. Volvemos la estética del botón a azul
            lv_label_set_text(lbl_accion, "Iniciar");
            lv_obj_set_style_bg_color(btn_accion, lv_palette_main(LV_PALETTE_BLUE), 0);
            
            // PREPARADO PARA LA LÓGICA: Acá en el próximo paso llamaremos a:
            // app_core_detener_grabacion_y_recortar();
            app_core_set_estado(ESTADO_REPOSO);
        }
    }
}

// ==========================================
// CREACIÓN DE LA PANTALLA
// ==========================================
void gui_crear_pantalla_pasos(void) {
    scr_pasos = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_pasos, lv_color_black(), 0);
    lbl_time_pasos = crear_reloj_superior(scr_pasos);

    // --- 1. CONTROLES SUPERIORES ---
    
    // Combo Actividad (Alineado a la izquierda)
    dd_actividad = lv_dropdown_create(scr_pasos);
    lv_dropdown_set_options(dd_actividad, "caminando\nsin_caminando");
    lv_obj_set_width(dd_actividad, 150);
    lv_obj_align(dd_actividad, LV_ALIGN_TOP_LEFT, 10, 50);

    // Combo Demora (Al medio)
    dd_demora = lv_dropdown_create(scr_pasos);
    lv_dropdown_set_options(dd_demora, "0s\n5s\n10s");
    lv_obj_set_width(dd_demora, 70);
    lv_obj_align(dd_demora, LV_ALIGN_TOP_LEFT, 170, 50);

    // Botón de Acción (Alineado a la derecha)
    btn_accion = lv_btn_create(scr_pasos);
    lv_obj_set_size(btn_accion, 100, 40);
    lv_obj_align(btn_accion, LV_ALIGN_TOP_LEFT, 250, 50);
    lv_obj_set_style_bg_color(btn_accion, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_accion, btn_accion_cb, LV_EVENT_ALL, NULL);
    
    lbl_accion = lv_label_create(btn_accion);
    lv_label_set_text(lbl_accion, "Iniciar");
    lv_obj_center(lbl_accion);

    // --- 2. GRÁFICOS (Empujados hacia abajo) ---
    chart_accel = lv_chart_create(scr_pasos);
    lv_obj_set_size(chart_accel, 330, 110);
    lv_obj_align(chart_accel, LV_ALIGN_TOP_MID, 0, 110);
    lv_chart_set_type(chart_accel, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_accel, LV_CHART_AXIS_PRIMARY_Y, -10922, 10922);
    ser_x = lv_chart_add_series(chart_accel, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser_y = lv_chart_add_series(chart_accel, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    ser_z = lv_chart_add_series(chart_accel, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    chart_mic = lv_chart_create(scr_pasos);
    lv_obj_set_size(chart_mic, 330, 110);
    lv_obj_align(chart_mic, LV_ALIGN_TOP_MID, 0, 235);
    lv_chart_set_type(chart_mic, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_mic, LV_CHART_AXIS_PRIMARY_Y, 0, 8000);
    ser_mic = lv_chart_add_series(chart_mic, lv_palette_main(LV_PALETTE_YELLOW), LV_CHART_AXIS_PRIMARY_Y);

    // --- 3. BOTÓN VOLVER ---
    lv_obj_t * btn_volverp = lv_btn_create(scr_pasos);
    lv_obj_set_size(btn_volverp, 140, 50);
    lv_obj_align(btn_volverp, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(btn_volverp, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_volverp, btn_volver_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * lbl_volverp = lv_label_create(btn_volverp);
    lv_label_set_text(lbl_volverp, "Volver");
    lv_obj_center(lbl_volverp);
}