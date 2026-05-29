#ifndef HW_MIC_H
#define HW_MIC_H

#include <stdint.h>
#include <stdbool.h>

void hw_mic_init(void);
bool hw_mic_read_dma(int16_t *buffer, int num_samples);

#endif // HW_MIC_H