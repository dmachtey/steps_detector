#include "hw_imu.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HW_IMU";

#define IMU_I2C_PORT I2C_NUM_0 // Usa el bus inicializado por hw_display
#define IMU_ADDR     0x6B

void hw_imu_init(void) {
    ESP_LOGI(TAG, "Configurando IMU QMI8658 en el bus I2C...");
    uint8_t ctrl1[2] = {0x02, 0x60};
    i2c_master_write_to_device(IMU_I2C_PORT, IMU_ADDR, ctrl1, 2, pdMS_TO_TICKS(100));

    uint8_t ctrl2[2] = {0x03, 0x23};
    i2c_master_write_to_device(IMU_I2C_PORT, IMU_ADDR, ctrl2, 2, pdMS_TO_TICKS(100));

    uint8_t ctrl7[2] = {0x08, 0x01};
    i2c_master_write_to_device(IMU_I2C_PORT, IMU_ADDR, ctrl7, 2, pdMS_TO_TICKS(100));

    vTaskDelay(pdMS_TO_TICKS(50));
}

bool hw_imu_read(int16_t *x, int16_t *y, int16_t *z) {
    uint8_t reg = 0x35;
    uint8_t raw_data[6];

    esp_err_t ret = i2c_master_write_read_device(IMU_I2C_PORT, IMU_ADDR, &reg, 1, raw_data, 6, pdMS_TO_TICKS(50));
    if (ret == ESP_OK) {
        *x = (int16_t)((raw_data[1] << 8) | raw_data[0]);
        *y = (int16_t)((raw_data[3] << 8) | raw_data[2]);
        *z = (int16_t)((raw_data[5] << 8) | raw_data[4]);
        return true;
    }
    return false;
}