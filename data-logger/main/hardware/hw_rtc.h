#ifndef HW_RTC_H
#define HW_RTC_H

#include <stdbool.h>
#include <time.h>

// Inicializa el sistema de tiempo interno con la zona horaria por defecto
void hw_rtc_init(void);

// Permite cambiar la zona horaria en caliente usando formato POSIX (ej. "ART3", "CET-1CEST")
void hw_rtc_set_timezone(const char *tz);

// Inicia la sincronización en segundo plano con servidores NTP
void hw_rtc_sync_ntp(void);

// Obtiene la hora actual. Retorna true si la hora ya fue sincronizada por NTP
bool hw_rtc_get_time(struct tm *timeinfo);

#endif // HW_RTC_H