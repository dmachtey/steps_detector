#include "gui_screens.h"
#include "gui_hw.h" // Necesario para usar gui_lock()
#include "../alarm_logic/alarm_core.h"


static lv_timer_t * timer_alarma = NULL;

// 1. Callback que devuelve la pantalla a GRIS a los 30 segundos
static void restaurar_color_cb(lv_timer_t * timer) {
    if (scr_alarma) {
        lv_obj_set_style_bg_color(scr_alarma, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    }
    timer_alarma = NULL;
    lv_timer_del(timer);
}

// 2. Escudo: Garantiza que SIEMPRE que entremos a la pantalla, sea GRIS
static void scr_alarma_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    // Cuando la pantalla ENTRA a la vista
    if(code == LV_EVENT_SCREEN_LOAD_START) {
        lv_obj_set_style_bg_color(scr_alarma, lv_palette_darken(LV_PALETTE_GREY, 3), 0);

        if (timer_alarma != NULL) {
            lv_timer_del(timer_alarma);
            timer_alarma = NULL;
        }

        // ¡Encendemos la IA!
        alarm_core_set_state(true);
    }
    // Cuando SALIMOS de la pantalla (porque tocaste volver)
    else if (code == LV_EVENT_SCREEN_UNLOAD_START) {
        // ¡Apagamos la IA!
        alarm_core_set_state(false);
    }
}

// 3. La función que va a llamar la Inteligencia Artificial (alarm_core.c)
void gui_trigger_alarma_visual(void) {
    if (gui_lock(-1)) {
        // Solo la pintamos de rojo si la pantalla activa es la de la alarma
        if (scr_alarma && lv_scr_act() == scr_alarma) {
            lv_obj_set_style_bg_color(scr_alarma, lv_palette_main(LV_PALETTE_RED), 0);

            // Reiniciamos la cuenta de 30 segundos
            if (timer_alarma != NULL) {
                lv_timer_del(timer_alarma);
            }
            timer_alarma = lv_timer_create(restaurar_color_cb, 30000, NULL);
        }
        gui_unlock();
    }
}

// 4. Creación inicial de la pantalla
void gui_crear_pantalla_alarma(void) {
    scr_alarma = lv_obj_create(NULL);

    // Le atamos el evento para que empiece gris al cargar
    lv_obj_add_event_cb(scr_alarma, scr_alarma_event_cb, LV_EVENT_SCREEN_LOAD_START, NULL);

    // Le seteamos el color GRIS de base (antes tenías RED acá)
    lv_obj_set_style_bg_color(scr_alarma, lv_palette_darken(LV_PALETTE_GREY, 3), 0);

    lbl_time_alarma = crear_reloj_superior(scr_alarma);

    lv_obj_t * btn_detener = lv_btn_create(scr_alarma);
    lv_obj_set_size(btn_detener, 200, 60);
    lv_obj_align(btn_detener, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_event_cb(btn_detener, btn_volver_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * lbl_detener = lv_label_create(btn_detener);
    lv_label_set_text(lbl_detener, "Desactivar");
    lv_obj_center(lbl_detener);
}
