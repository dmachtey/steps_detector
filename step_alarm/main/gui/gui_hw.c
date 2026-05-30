#include "gui_hw.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "GUI_HW";

// --- VARIABLES DEL MOTOR LVGL ---
static SemaphoreHandle_t lvgl_mux = NULL;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;

// ==========================================
// CALLBACKS INTERNOS DE LVGL
// ==========================================
static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

static void touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)drv->user_data;
    uint16_t tp_x, tp_y;
    uint8_t tp_cnt = 0;

    esp_lcd_touch_read_data(tp);
    if (esp_lcd_touch_get_coordinates(tp, &tp_x, &tp_y, NULL, &tp_cnt, 1) && tp_cnt > 0) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void tick_timer_cb(void *arg) { lv_tick_inc(2); }

// Tarea principal que procesa la interfaz gráfica
static void lvgl_port_task(void *arg) {
    while (1) {
        uint32_t delay = 500;
        if (gui_lock(-1)) {
            delay = lv_timer_handler();
            gui_unlock();
        }
        if (delay > 500) delay = 500;
        else if (delay < 5) delay = 5;
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

// ==========================================
// API PÚBLICA DE HARDWARE GUI
// ==========================================
bool gui_lock(int timeout_ms) {
    if (lvgl_mux == NULL) return false;
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

void gui_unlock(void) {
    xSemaphoreGive(lvgl_mux);
}

void gui_hw_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle) {
    ESP_LOGI(TAG, "Inicializando Motor LVGL...");
    lv_init();

    // Doble Buffer en memoria DMA (Crítico para pantallas QSPI)
    lv_color_t *buf1 = heap_caps_malloc(368 * 112 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(368 * 112 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, 368 * 112);

    // Inicializar el driver del display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 368;
    disp_drv.ver_res = 448;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // Inicializar el driver del touch
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = touch_cb;
    indev_drv.user_data = touch_handle;
    lv_indev_drv_register(&indev_drv);

    // Timer para que LVGL sepa medir el tiempo (animaciones, pulsaciones largas)
    const esp_timer_create_args_t tick_args = { .callback = &tick_timer_cb, .name = "lvgl_tick" };
    esp_timer_handle_t tick_timer;
    esp_timer_create(&tick_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, 2 * 1000);

    // Crear el Semáforo Mutex y lanzar la Tarea principal
    lvgl_mux = xSemaphoreCreateMutex();
    xTaskCreate(lvgl_port_task, "LVGL", 6144, NULL, 2, NULL);

    ESP_LOGI(TAG, "Motor gráfico iniciado y tarea corriendo.");
}
