#ifndef HW_IMU_H
#define HW_IMU_H

#include <stdint.h>
#include <stdbool.h>

void hw_imu_init(void);
bool hw_imu_read(int16_t *x, int16_t *y, int16_t *z);

#endif // HW_IMU_H