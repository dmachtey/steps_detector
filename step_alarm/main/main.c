#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "gui_hw.h"

#include "hardware.h"
#include "gui.h"
#include "core/app_core.h"
#include "alarm_logic/alarm_core.h"
#include "config.h"


// Inclusión de la cabecera del simulador
#include "mock_sensors.h"

static const char *TAG = "MAIN";

void app_main(void) {
    esp_log_level_set("lcd_panel.io.i2c", ESP_LOG_NONE);
    esp_log_level_set("FT5x06", ESP_LOG_NONE);

    ESP_LOGI(TAG, "==== INICIANDO DATALOGGER ====");

    // 0. Inicializar la Memoria No Volátil (NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 1. Levantar el Hardware
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_touch_handle_t touch_handle = NULL;
    hardware_init(&panel_handle, &touch_handle);

    // 2. Levantar la Interfaz Gráfica
    gui_hw_init(panel_handle, touch_handle);
    if (gui_lock(-1)) {
        gui_crear_pantallas();
        gui_unlock();
    }

    // 3. Inicializar la Lógica (SD, Colas y WiFi)
    // IMPORTANTE: Dentro de app_core_init() NO se deben iniciar hw_imu ni hw_mic si el simulador está en 1.
    app_core_init();

    ESP_LOGI(TAG, "==== ARRANQUE EXITOSO ====");

    // 4. Iniciar el vigilante del pontón (El Cerebro)
    alarm_core_start_task();

    // Activamos la alarma automáticamente para la prueba
    alarm_core_set_state(true);

#if MODO_SIMULADOR_SD == 1
    // 5. Iniciar el Impostor (Inyecta datos a las colas desde la SD)
    xTaskCreate(mock_sensors_task, "mock_sensors", 4096, NULL, 5, NULL);
#endif

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
