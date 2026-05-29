#ifndef HW_SD_H
#define HW_SD_H

#include <stdbool.h>

// Ruta raíz donde se montará la tarjeta SD en el sistema de archivos
#define MOUNT_POINT "/sdcard"

// Inicializa el bus SPI, la tarjeta SD y monta el sistema FatFS
bool hw_sd_init(void);

// Desmonta la tarjeta de forma segura
void hw_sd_deinit(void);

#endif // HW_SD_H