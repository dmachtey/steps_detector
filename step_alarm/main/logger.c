#include "logger.h"
#include "hardware/hw_rtc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>

static const char *TAG = "LOGGER";

// Configuración de recortes
#define SAMPLE_RATE_MIC 8000
#define SAMPLE_RATE_IMU 50
#define SECONDS_TO_CROP 3

// Estados internos
typedef enum {
    LOG_IDLE,
    LOG_WAITING_DELAY,
    LOG_RECORDING
} logger_state_t;

static volatile logger_state_t log_state = LOG_IDLE;
static uint64_t start_time_us = 0;
static int current_delay_sec = 0;

// Archivos
static FILE *f_csv = NULL;
static FILE *f_wav = NULL;

// Buffer circular de offsets para el CSV (50Hz * 3s = 150 elementos)
#define CSV_CROP_SAMPLES (SAMPLE_RATE_IMU * SECONDS_TO_CROP)
static off_t csv_offsets[CSV_CROP_SAMPLES];
static int csv_offset_idx = 0;
static int csv_total_samples = 0;

// Contadores de Audio
static uint32_t wav_data_bytes = 0;

// --- ESTRUCTURA DE CABECERA WAV (44 Bytes) ---
#pragma pack(push, 1)
typedef struct {
    char chunkId[4];        // "RIFF"
    uint32_t chunkSize;     // Tamaño archivo - 8
    char format[4];         // "WAVE"
    char subchunk1Id[4];    // "fmt "
    uint32_t subchunk1Size; // 16
    uint16_t audioFormat;   // 1 (PCM)
    uint16_t numChannels;   // 1 (Mono)
    uint32_t sampleRate;    // 8000
    uint32_t byteRate;      // 8000 * 2
    uint16_t blockAlign;    // 2
    uint16_t bitsPerSample; // 16
    char subchunk2Id[4];    // "data"
    uint32_t subchunk2Size; // Tamaño de los datos
} wav_header_t;
#pragma pack(pop)

// --- FUNCIONES PRIVADAS ---

static void write_wav_header(FILE *f, uint32_t data_size) {
    wav_header_t header = {
        .chunkId = {'R','I','F','F'},
        .chunkSize = 36 + data_size,
        .format = {'W','A','V','E'},
        .subchunk1Id = {'f','m','t',' '},
        .subchunk1Size = 16,
        .audioFormat = 1,
        .numChannels = 1,
        .sampleRate = SAMPLE_RATE_MIC,
        .byteRate = SAMPLE_RATE_MIC * 2,
        .blockAlign = 2,
        .bitsPerSample = 16,
        .subchunk2Id = {'d','a','t','a'},
        .subchunk2Size = data_size
    };
    fseek(f, 0, SEEK_SET);
    fwrite(&header, 1, sizeof(wav_header_t), f);
}

static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static void logger_imprimir_dir(void) {
    DIR *dir = opendir("/sdcard");
    if (dir == NULL) {
        ESP_LOGE(TAG, "Error: No se pudo abrir /sdcard. ¿Está montada la memoria?");
        return;
    }

    ESP_LOGI(TAG, "=== CONTENIDO DE /sdcard ===");
    struct dirent *ent;
    struct stat st;
    char full_path[300];

    while ((ent = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "/sdcard/%s", ent->d_name);
        if (stat(full_path, &st) == 0) {
            ESP_LOGI(TAG, " -> %-35s | %8ld bytes", ent->d_name, (long)st.st_size);
        } else {
            ESP_LOGI(TAG, " -> %s", ent->d_name);
        }
    }
    closedir(dir);
    ESP_LOGI(TAG, "============================");
}

// --- API PÚBLICA ---
void logger_init(void) {
    log_state = LOG_IDLE;
    ESP_LOGI(TAG, "Módulo Logger inicializado.");
    logger_imprimir_dir();
}

bool logger_is_recording(void) {
    return (log_state == LOG_RECORDING);
}

void logger_start(const char* actividad, int demora_segundos) {
    if (log_state != LOG_IDLE) return;

    struct tm timeinfo;
    hw_rtc_get_time(&timeinfo);

    char base_filename[128];
    snprintf(base_filename, sizeof(base_filename), "/sdcard/%s_%04d%02d%02d_%02d%02d%02d",
             actividad,
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    char csv_path[150], wav_path[150];
    snprintf(csv_path, sizeof(csv_path), "%s_acelerometro.csv", base_filename);
    snprintf(wav_path, sizeof(wav_path), "%s_audio.wav", base_filename);

    f_csv = fopen(csv_path, "w");
    f_wav = fopen(wav_path, "wb");

    if (!f_csv || !f_wav) {
        ESP_LOGE(TAG, "Error abriendo archivos en SD.");
        if (f_csv) fclose(f_csv);
        if (f_wav) fclose(f_wav);
        return;
    }

    csv_offset_idx = 0;
    csv_total_samples = 0;
    wav_data_bytes = 0;

    write_wav_header(f_wav, 0);
    fprintf(f_csv, "timestamp_ms,accel_x,accel_y,accel_z\n");

    current_delay_sec = demora_segundos;
    start_time_us = get_time_us();

    if (current_delay_sec > 0) {
        log_state = LOG_WAITING_DELAY;
        ESP_LOGI(TAG, "Logger en espera: %d segundos...", current_delay_sec);
    } else {
        log_state = LOG_RECORDING;
        ESP_LOGI(TAG, "Grabando en: %s", base_filename);
    }
}

// === VERSIÓN PARCHADA DE LOGGER_STOP CON SEMÁFORO DE TIEMPO ===
void logger_stop(void) {
    if (log_state != LOG_RECORDING) {
        log_state = LOG_IDLE;
        return;
    }

    // 1. CORTAR EL FLUJO DE DATOS INMEDIATAMENTE
    log_state = LOG_IDLE;

    // 2. Dar tiempo (50ms) para que las tareas del IMU y Micrófono terminen
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Deteniendo grabación y aplicando recorte de %d segundos...", SECONDS_TO_CROP);

    // --- 1. RECORTAR CSV ---
    if (f_csv) {
        if (csv_total_samples > CSV_CROP_SAMPLES) {
            off_t truncate_pos = csv_offsets[csv_offset_idx];

            // Forzar guardado en SD antes del recorte
            fflush(f_csv);
            ftruncate(fileno(f_csv), truncate_pos);
        }
        fclose(f_csv);
        f_csv = NULL;
    }

    // --- 2. RECORTAR Y REPARAR WAV ---
    if (f_wav) {
        uint32_t bytes_to_crop = SAMPLE_RATE_MIC * 2 * SECONDS_TO_CROP;
        if (wav_data_bytes > bytes_to_crop) {
            wav_data_bytes -= bytes_to_crop;
        } else {
            wav_data_bytes = 0;
        }

        // Forzar guardado del audio residual en SD
        fflush(f_wav);

        // Cortar físicamente el archivo dejando solo la cabecera (44) + el audio útil
        ftruncate(fileno(f_wav), 44 + wav_data_bytes);

        // Volver al inicio del archivo para actualizar la cabecera
        fseek(f_wav, 0, SEEK_SET);
        write_wav_header(f_wav, wav_data_bytes);

        // Volcar por última vez para asegurar la cabecera
        fflush(f_wav);

        fclose(f_wav);
        f_wav = NULL;
    }

    ESP_LOGI(TAG, "Archivos guardados, cerrados y listos para analizar.");
}

void logger_feed_imu(int16_t x, int16_t y, int16_t z) {
    if (log_state == LOG_WAITING_DELAY) {
        if ((get_time_us() - start_time_us) / 1000000ULL >= current_delay_sec) {
            log_state = LOG_RECORDING;
            start_time_us = get_time_us();
            ESP_LOGI(TAG, "¡Retardo superado, iniciando grabación real!");
        }
    }

    if (log_state == LOG_RECORDING && f_csv) {
        csv_offsets[csv_offset_idx] = ftello(f_csv);

        uint32_t ts_ms = (uint32_t)((get_time_us() - start_time_us) / 1000);
        fprintf(f_csv, "%lu,%d,%d,%d\n", ts_ms, x, y, z);

        csv_offset_idx = (csv_offset_idx + 1) % CSV_CROP_SAMPLES;
        csv_total_samples++;
    }
}

void logger_feed_mic(int16_t *buffer, size_t num_samples) {
    if (log_state == LOG_RECORDING && f_wav) {

        // num_samples trae la cantidad de lecturas (L + R).
        // Como queremos Mono real, la cantidad final será la mitad.
        size_t mono_samples = num_samples / 2;

        // Compactamos el buffer sobre sí mismo, guardando solo el canal Izquierdo
        for (size_t i = 0; i < mono_samples; i++) {
            buffer[i] = buffer[i * 2];
            // Nota: Si al probarlo escuchas silencio, cambiá el índice a: buffer[i * 2 + 1]
            // (depende de a qué pin L/R tengas soldado el micrófono)
        }

        // Ahora escribimos en la SD la mitad de los bytes (Mono real)
        size_t bytes_to_write = mono_samples * sizeof(int16_t);
        fwrite(buffer, 1, bytes_to_write, f_wav);
        wav_data_bytes += bytes_to_write;
    }
}
