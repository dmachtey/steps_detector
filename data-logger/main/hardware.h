#ifndef HARDWARE_H
#define HARDWARE_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

// Inicializa todos los periféricos de la placa
void hardware_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch);

#endif // HARDWARE_H