#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Solo necesitamos que conozca el tipo de dato que usa FreeRTOS para las tareas
void mock_sensors_task(void *pvParameters);

#ifdef __cplusplus
}
#endif