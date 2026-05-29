#include "logger.h"
#include "hardware/hw_rtc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h> // Para ftruncate()
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>

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

static uint64_t get_time_us() {
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
    char full_path[300]; // Buffer para armar la ruta completa

    while ((ent = readdir(dir)) != NULL) {
        // Armamos la ruta absoluta del archivo
        snprintf(full_path, sizeof(full_path), "/sdcard/%s", ent->d_name);

        // Consultamos los metadatos del archivo
        if (stat(full_path, &st) == 0) {
            // Imprimimos el nombre alineado a la izquierda (35 caracteres) y el tamaño en bytes
            ESP_LOGI(TAG, " -> %-35s | %8ld bytes", ent->d_name, (long)st.st_size);
        } else {
            // Fallback por si stat falla (raro, pero seguro)
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

    // Sonda temporal al arrancar la placa
    logger_imprimir_dir();
}

bool logger_is_recording(void) {
    return (log_state == LOG_RECORDING);
}

void logger_start(const char* actividad, int demora_segundos) {
    if (log_state != LOG_IDLE) return;

    struct tm timeinfo;
    hw_rtc_get_time(&timeinfo); // Necesitamos la hora actual para el nombre

    char base_filename[128];
    // Formato: /sdcard/actividad_YYYYMMDD_HHMMSS
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

    // Preparar variables de recorte
    csv_offset_idx = 0;
    csv_total_samples = 0;
    wav_data_bytes = 0;

    // Escribir cabecera vacía del WAV
    write_wav_header(f_wav, 0);

    // Escribir cabecera del CSV
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

void logger_stop(void) {
    if (log_state == LOG_IDLE) return;
    log_state = LOG_IDLE;

    if (!f_csv || !f_wav) return;

    ESP_LOGI(TAG, "Deteniendo grabación y aplicando recorte de %ds...", SECONDS_TO_CROP);

    // --- RECORTAR CSV ---
    if (csv_total_samples > CSV_CROP_SAMPLES) {
        off_t truncate_pos = csv_offsets[csv_offset_idx]; // El offset más antiguo del buffer circular
        ftruncate(fileno(f_csv), truncate_pos);
    }

    // --- RECORTAR WAV ---
    uint32_t bytes_to_crop = SAMPLE_RATE_MIC * 2 * SECONDS_TO_CROP;
    if (wav_data_bytes > bytes_to_crop) {
        wav_data_bytes -= bytes_to_crop;
        // 1. Actualizar la cabecera real
        write_wav_header(f_wav, wav_data_bytes);
        // 2. Recortar el archivo físico (Cabecera 44 bytes + Datos)
        ftruncate(fileno(f_wav), 44 + wav_data_bytes);
    }

    fclose(f_csv);
    fclose(f_wav);
    f_csv = NULL;
    f_wav = NULL;

    ESP_LOGI(TAG, "Archivos guardados y cerrados correctamente.");
}

void logger_feed_imu(int16_t x, int16_t y, int16_t z) {
    if (log_state == LOG_WAITING_DELAY) {
        if ((get_time_us() - start_time_us) / 1000000ULL >= current_delay_sec) {
            log_state = LOG_RECORDING;
            start_time_us = get_time_us(); // Reiniciamos el tiempo para el CSV
            ESP_LOGI(TAG, "¡Retardo superado, iniciando grabación real!");
        }
    }

    if (log_state == LOG_RECORDING && f_csv) {
        // Guardamos en qué byte estamos por empezar a escribir esta línea
        csv_offsets[csv_offset_idx] = ftello(f_csv);

        uint32_t ts_ms = (uint32_t)((get_time_us() - start_time_us) / 1000);
        fprintf(f_csv, "%lu,%d,%d,%d\n", ts_ms, x, y, z);

        // Avanzamos el anillo
        csv_offset_idx = (csv_offset_idx + 1) % CSV_CROP_SAMPLES;
        csv_total_samples++;
    }
}

void logger_feed_mic(int16_t *buffer, size_t num_samples) {
    if (log_state == LOG_RECORDING && f_wav) {
        size_t bytes_to_write = num_samples * sizeof(int16_t);
        fwrite(buffer, 1, bytes_to_write, f_wav);
        wav_data_bytes += bytes_to_write;
    }
}
