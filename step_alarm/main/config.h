#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>

// ==============================================================================
// 🚩 CONFIGURACIÓN MAESTRA DEL SISTEMA
// ==============================================================================

// MODO DE OPERACIÓN
// 1 = Lee los CSV/WAV desde la SD (Simulador de Laboratorio)
// 0 = Usa el Hardware Real (IMU I2C y Micrófono I2S en el pontón)
#define MODO_SIMULADOR_SD 1

// RUTAS DEL SISTEMA
#define SD_MOUNT_POINT "/sdcard/raw-data"


// ==============================================================================
// 🌐 VARIABLES GLOBALES (Declaración)
// ==============================================================================

// Colas de comunicación (Productor -> Consumidor)
extern QueueHandle_t alarm_imu_queue;
extern QueueHandle_t alarm_mic_queue;