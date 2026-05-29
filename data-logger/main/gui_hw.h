#ifndef GUI_HW_H
#define GUI_HW_H

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include <stdbool.h>

// Inicializa el motor LVGL y los buffers DMA
void gui_hw_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle);

// Control de concurrencia para acceso seguro a la pantalla
bool gui_lock(int timeout_ms);
void gui_unlock(void);

#endif // GUI_HW_H
