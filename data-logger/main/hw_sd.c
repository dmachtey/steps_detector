#include "hw_sd.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"

static const char *TAG = "HW_SD";

// =========================================================
// DEFINICIÓN DE PINES SDMMC (Extraídos de sdkconfig.defaults)
// =========================================================
#define SDMMC_PIN_CMD  1
#define SDMMC_PIN_CLK  2
#define SDMMC_PIN_D0   3

static sdmmc_card_t *sd_card = NULL;

bool hw_sd_init(void) {
    esp_err_t ret;
    ESP_LOGI(TAG, "Inicializando Tarjeta SD vía SDMMC...");

    // 1. Configurar las opciones de montaje
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // 2. Configurar el host SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    // 3. Configurar el slot (1-bit mode)
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    // Asignar los pines manualmente
    slot_config.clk = SDMMC_PIN_CLK;
    slot_config.cmd = SDMMC_PIN_CMD;
    slot_config.d0  = SDMMC_PIN_D0;

    // Pull-ups internas obligatorias para SDMMC
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // 4. Montar la partición FatFS
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &sd_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Fallo al montar el sistema de archivos. Comprueba que esté en FAT32.");
        } else {
            ESP_LOGE(TAG, "Fallo al inicializar la SD (%s).", esp_err_to_name(ret));
        }
        return false;
    }

    ESP_LOGI(TAG, "¡SD montada exitosamente en %s!", MOUNT_POINT);
    sdmmc_card_print_info(stdout, sd_card); // Imprime el tamaño y modelo
    return true;
}

void hw_sd_deinit(void) {
    if (sd_card != NULL) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, sd_card);
        ESP_LOGI(TAG, "Tarjeta SD desmontada correctamente.");
        sd_card = NULL;
    }
}
