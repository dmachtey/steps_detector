#ifndef HW_DISPLAY_H
#define HW_DISPLAY_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

// Inicializa el bus I2C compartido, la pantalla por SPI y el controlador Touch
void hw_display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch);

#endif // HW_DISPLAY_H