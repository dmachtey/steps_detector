#include "hw_display.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_sh8601.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_io_expander_tca9554.h"

static const char *TAG = "HW_DISPLAY";

#define LCD_HOST SPI2_HOST
#define TOUCH_HOST I2C_NUM_0

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    {0x11, (uint8_t[]){0x00}, 0, 120}, {0x44, (uint8_t[]){0x01, 0xD1}, 2, 0},
    {0x35, (uint8_t[]){0x00}, 1, 0},   {0x53, (uint8_t[]){0x20}, 1, 10},
    {0x2A, (uint8_t[]){0x00, 0x00, 0x01, 0x6F}, 4, 0}, {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xBF}, 4, 0},
    {0x51, (uint8_t[]){0x00}, 1, 10},  {0x29, (uint8_t[]){0x00}, 0, 10},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
};

void hw_display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch) {
    ESP_LOGI(TAG, "Inicializando I2C Compartido (SCL:14, SDA:15)...");
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER, .sda_io_num = 15, .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = 14, .scl_pullup_en = GPIO_PULLUP_ENABLE, .master.clk_speed = 400 * 1000,
    };
    i2c_param_config(TOUCH_HOST, &i2c_conf);
    i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0);

    ESP_LOGI(TAG, "Configurando Expansor de Energía TCA9554...");
    esp_io_expander_handle_t io_expander = NULL;
    esp_io_expander_new_i2c_tca9554(TOUCH_HOST, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &io_expander);
    esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_7, IO_EXPANDER_OUTPUT);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0, 0);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_1, 0);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0, 1);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_1, 1);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 1);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_7, 1); // ¡SD ENCENDIDA!



    ESP_LOGI(TAG, "Inicializando QSPI para la Pantalla...");
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(11, 4, 5, 6, 7, 368 * 448 * 2);
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(12, NULL, NULL);
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    sh8601_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds, .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = { .use_qspi_interface = 1 },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16, .vendor_config = &vendor_config,
    };
    esp_lcd_new_panel_sh8601(io_handle, &panel_config, out_panel);
    esp_lcd_panel_reset(*out_panel);
    esp_lcd_panel_init(*out_panel);
    esp_lcd_panel_disp_on_off(*out_panel, true);

    ESP_LOGI(TAG, "Inicializando Touch FT5x06...");
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle);
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = 368, .y_max = 448, .rst_gpio_num = -1, .int_gpio_num = 21,
        .flags = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, out_touch);
}
