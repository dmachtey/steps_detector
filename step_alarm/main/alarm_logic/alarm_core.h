#ifndef ALARM_CORE_H
#define ALARM_CORE_H
#include <stdbool.h>

// Inicia la tarea en segundo plano que vigila el pontón
void alarm_core_start_task(void);
void alarm_core_set_state(bool state);


#endif
