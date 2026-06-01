#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mock_sensors.h"
#include "alarm_core.h" // O el archivo donde esté definido imu_data_t
#include "config.h"

static const char *TAG = "MOCK_SENSORS";

// Definimos la estructura acá si no la tenés global en un .h
#ifndef IMU_DATA_T_DEFINED
#define IMU_DATA_T_DEFINED
typedef struct { int16_t x; int16_t y; int16_t z; } imu_data_t;
#endif

void mock_sensors_task(void *pvParameters) {
    ESP_LOGW(TAG, "INICIANDO MODO SIMULADOR DESDE SD...");

    DIR *dir = opendir(SD_MOUNT_POINT);
    if (!dir) {
        ESP_LOGE(TAG, "Error al abrir el directorio de la SD: %s", SD_MOUNT_POINT);
        vTaskDelete(NULL);
    }

    struct dirent *ent;
    char path_csv[256];
    char path_wav[256];
    char line[128];
    int16_t mic_chunk[160];

    while ((ent = readdir(dir)) != NULL) {
        if (strstr(ent->d_name, "_acelerometro.csv")) {

            // 1. Extraer el "Basename" limpio
            char basename[128];
            strncpy(basename, ent->d_name, sizeof(basename));
            char *suffix = strstr(basename, "_acelerometro.csv");
            if (suffix) *suffix = '\0'; // Cortamos el string justo acá

            // 2. Armar las rutas usando el basename
            snprintf(path_csv, sizeof(path_csv), "%s/%s", SD_MOUNT_POINT, ent->d_name);
            snprintf(path_wav, sizeof(path_wav), "%s/%s_audio.wav", SD_MOUNT_POINT, basename);

            ESP_LOGI(TAG, "==========================================");
            ESP_LOGI(TAG, "▶ INYECTANDO: %s", basename);
            ESP_LOGI(TAG, "==========================================");

            FILE *f_csv = fopen(path_csv, "r");
            FILE *f_wav = fopen(path_wav, "rb");

            if (!f_csv || !f_wav) {
                ESP_LOGE(TAG, "Falta el par WAV para %s. Saltando...", ent->d_name);
                if (f_csv) fclose(f_csv);
                if (f_wav) fclose(f_wav);
                continue;
            }

            // Descartar encabezados
            fgets(line, sizeof(line), f_csv); // Ignora: "timestamp_ms,accel_x,accel_y,accel_z"
            fseek(f_wav, 44, SEEK_SET);       // Ignora el header WAV de 44 bytes

            // Bucle de Inyección (20ms)
            while (fgets(line, sizeof(line), f_csv) &&
                   fread(mic_chunk, sizeof(int16_t), 160, f_wav) == 160) {

                imu_data_t imu_data = {0, 0, 0};

                // LA MÁSCARA EXACTA:
                // %*d -> Ignora el timestamp_ms
                // ,   -> Salta la coma
                // %hd -> Lee int16_t (short) para X, Y, Z
                if (sscanf(line, "%*d,%hd,%hd,%hd", &imu_data.x, &imu_data.y, &imu_data.z) == 3) {

                    xQueueSend(alarm_imu_queue, &imu_data, portMAX_DELAY);
                    xQueueSend(alarm_mic_queue, mic_chunk, portMAX_DELAY);

                    // Esperamos 20ms para simular que somos el hardware real (50Hz)
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
            }

            // CORRECCIÓN APLICADA ACÁ: Usamos "basename" en vez de "ent->d_name"
            ESP_LOGI(TAG, "⏹ FIN DEL ARCHIVO: %s. Siguiente en 3 seg...", basename);
            fclose(f_csv);
            fclose(f_wav);
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }

    closedir(dir);
    ESP_LOGW(TAG, "✅ TODOS LOS ARCHIVOS PROBADOS. FIN DE LA SIMULACIÓN.");
    vTaskDelete(NULL);
}