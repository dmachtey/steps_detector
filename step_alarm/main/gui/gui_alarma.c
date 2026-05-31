#include "gui_screens.h"
#include "gui_hw.h" // Necesario para gui_lock() y gui_unlock()

static lv_timer_t * timer_alarma = NULL;

// Callback que se ejecuta automáticamente a los 30 segundos
static void restaurar_color_cb(lv_timer_t * timer) {
    if (scr_alarma) {
        // Volver al color neutro (Gris oscuro)
        lv_obj_set_style_bg_color(scr_alarma, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    }
    timer_alarma = NULL; // Limpiamos el puntero
    lv_timer_del(timer); // Destruimos el timer para que no se repita
}

// API Pública para que la tarea de la IA dispare la pantalla
void gui_trigger_alarma_visual(void) {
    // Como esta función la llama la IA desde otra tarea, HAY que bloquear el motor gráfico
    if (gui_lock(-1)) {
        if (scr_alarma) {
            // 1. Pintar la pantalla de ROJO ALERTA
            lv_obj_set_style_bg_color(scr_alarma, lv_palette_main(LV_PALETTE_RED), 0);

            // 2. Si ya había un timer corriendo (porque pisaron dos veces seguidas), lo borramos
            if (timer_alarma != NULL) {
                lv_timer_del(timer_alarma);
            }
            
            // 3. Crear el timer de 30.000 milisegundos (30 segundos)
            timer_alarma = lv_timer_create(restaurar_color_cb, 30000, NULL);
        }
        gui_unlock();
    }
}

void gui_crear_pantalla_alarma(void) {
    scr_alarma = lv_obj_create(NULL);
    
    // Color por defecto: NEUTRO (Gris muy oscuro)
    lv_obj_set_style_bg_color(scr_alarma, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    
    lbl_time_alarma = crear_reloj_superior(scr_alarma);

    lv_obj_t * btn_detener = lv_btn_create(scr_alarma);
    lv_obj_set_size(btn_detener, 200, 60);
    lv_obj_align(btn_detener, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn_detener, btn_volver_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t * lbl_detener = lv_label_create(btn_detener);
    lv_label_set_text(lbl_detener, "Desactivar / Volver");
    lv_obj_center(lbl_detener);
}