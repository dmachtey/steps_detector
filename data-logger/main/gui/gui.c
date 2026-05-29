#include "gui.h"
#include "gui_hw.h"
#include "gui_screens.h"
#include "gui_hostname.h"
#include <stdio.h>
#include "app_core.h" // Asegurate de que esté este include arriba de todo
#include "esp_log.h"
#include "../hardware/hw_rtc.h"

// Declaración REAL de los punteros (el espacio en memoria existe acá)
lv_obj_t * scr_main = NULL;
lv_obj_t * scr_alarma = NULL;
lv_obj_t * scr_config = NULL;
lv_obj_t * scr_pasos = NULL;
lv_obj_t * scr_wifi = NULL;

lv_obj_t * lbl_time_main = NULL;
lv_obj_t * lbl_time_alarma = NULL;
lv_obj_t * lbl_time_config = NULL;
lv_obj_t * lbl_time_pasos = NULL;
lv_obj_t * lbl_time_wifi = NULL;



// El "Vigilante" de inactividad
static void inactividad_timer_cb(lv_timer_t * timer) {
    if (lv_scr_act() == scr_main) {
        if (lv_disp_get_inactive_time(NULL) >= 5000) {
            ESP_LOGI("GUI", "5s de inactividad en Main. Activando alarma...");

            // Resetea el tiempo de inactividad para que no se dispare en loop
            lv_disp_trig_activity(NULL);

            // Cambia de pantalla e inicia el registro
            lv_scr_load(scr_alarma);
            app_core_iniciar_grabacion();
        }
    }
}

static void reloj_timer_cb(lv_timer_t * timer) {
    struct tm timeinfo;

    // Si el RTC ya tiene una hora válida (ya se sincronizó por NTP)
    if (hw_rtc_get_time(&timeinfo)) {
        char buf[32]; // Agrandamos el buffer para que entre la fecha y la hora

        // Formateamos: DD/MM/YYYY HH:MM
        snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d",
                 timeinfo.tm_mday,
                 timeinfo.tm_mon + 1,        // Los meses arrancan en 0
                 timeinfo.tm_year + 1900,    // Años desde 1900
                 timeinfo.tm_hour,
                 timeinfo.tm_min);

        // Actualizamos directamente. ¡Sin gui_lock() porque LVGL ya nos está protegiendo!
        if (lbl_time_main) lv_label_set_text(lbl_time_main, buf);
        if (lbl_time_alarma) lv_label_set_text(lbl_time_alarma, buf);
        if (lbl_time_config) lv_label_set_text(lbl_time_config, buf);
        if (lbl_time_pasos) lv_label_set_text(lbl_time_pasos, buf);
        if (lbl_time_wifi) lv_label_set_text(lbl_time_wifi, buf);
    }
}

// Helper global: Crea el reloj en la esquina superior derecha
lv_obj_t * crear_reloj_superior(lv_obj_t * parent) {
    lv_obj_t * lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "00:00");
    lv_obj_set_style_text_color(lbl, lv_color_make(220, 220, 220), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_RIGHT, -30, 15);
    return lbl;
}

// Helper global: Botón Volver genérico al Main
void btn_volver_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_scr_load(scr_main);
    }
}

// API: Actualiza todos los relojes de todas las pantallas
void gui_set_time(int hora, int minuto) {
    if (gui_lock(-1)) {
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", hora, minuto);
        if (lbl_time_main) lv_label_set_text(lbl_time_main, buf);
        if (lbl_time_alarma) lv_label_set_text(lbl_time_alarma, buf);
        if (lbl_time_config) lv_label_set_text(lbl_time_config, buf);
        if (lbl_time_pasos) lv_label_set_text(lbl_time_pasos, buf);
        if (lbl_time_wifi) lv_label_set_text(lbl_time_wifi, buf);
        gui_unlock();
    }
}

// El Main llama a esta función al arrancar
void gui_crear_pantallas(void) {
    gui_crear_pantalla_main();
    gui_crear_pantalla_alarma();
    gui_crear_pantalla_config();
    gui_crear_pantalla_hostname();
    gui_crear_pantalla_pasos();
    gui_crear_pantalla_wifi();
    lv_timer_create(inactividad_timer_cb, 500, NULL);
    // <-- NUEVO: El actualizador del reloj (corre cada 1000ms)
    lv_timer_create(reloj_timer_cb, 1000, NULL);
    lv_scr_load(scr_main); // Pantalla inicial
}
