#include "hardware.h"
#include "esp_log.h"
#include "hw_rtc.h"

// Inclusiones de tus nuevos módulos divididos
#include "hw_display.h"
#include "hw_imu.h"
#include "hw_mic.h"
#include "hw_sd.h"

static const char *TAG = "HARDWARE_MANAGER";

void hardware_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch) {
    ESP_LOGI(TAG, "--- INICIANDO SECUENCIA BSP ---");

    // 1. Pantalla y Touch
    hw_display_init(out_panel, out_touch);

    // 2. Sensores
    hw_imu_init();

    // 3. Audio DMA
    hw_mic_init();

    // 4. Memoria SD
    if (!hw_sd_init()) {
        ESP_LOGW(TAG, "Sistema iniciado sin memoria SD. No se podrán guardar logs largos.");
    }

    hw_rtc_init(); // <-- Inicializa el reloj base

    ESP_LOGI(TAG, "--- BSP INICIALIZADO CORRECTAMENTE ---");
}
