#include "gui_screens.h"
#include "gui_hw.h" // Para usar gui_lock()

// Variables de los gráficos exclusivas de esta pantalla
static lv_obj_t * chart_accel;
static lv_chart_series_t * ser_x;
static lv_chart_series_t * ser_y;
static lv_chart_series_t * ser_z;
static lv_obj_t * chart_mic;
static lv_chart_series_t * ser_mic;

// La API que app_core llamará para actualizar los gráficos
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

void gui_crear_pantalla_pasos(void) {
    scr_pasos = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_pasos, lv_color_black(), 0);
    lbl_time_pasos = crear_reloj_superior(scr_pasos);

    chart_accel = lv_chart_create(scr_pasos);
    lv_obj_set_size(chart_accel, 320, 110);
    lv_obj_align(chart_accel, LV_ALIGN_TOP_MID, 0, 100);
    lv_chart_set_type(chart_accel, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_accel, LV_CHART_AXIS_PRIMARY_Y, -10922, 10922);
    ser_x = lv_chart_add_series(chart_accel, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser_y = lv_chart_add_series(chart_accel, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    ser_z = lv_chart_add_series(chart_accel, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    chart_mic = lv_chart_create(scr_pasos);
    lv_obj_set_size(chart_mic, 320, 110);
    lv_obj_align(chart_mic, LV_ALIGN_TOP_MID, 0, 230);
    lv_chart_set_type(chart_mic, LV_CHART_TYPE_LINE);
    lv_chart_set_range(chart_mic, LV_CHART_AXIS_PRIMARY_Y, 0, 8000);
    ser_mic = lv_chart_add_series(chart_mic, lv_palette_main(LV_PALETTE_YELLOW), LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t * btn_volverp = lv_btn_create(scr_pasos);
    lv_obj_set_size(btn_volverp, 140, 50);
    lv_obj_align(btn_volverp, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(btn_volverp, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_volverp, btn_volver_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * lbl_volverp = lv_label_create(btn_volverp);
    lv_label_set_text(lbl_volverp, "Volver");
    lv_obj_center(lbl_volverp);
}