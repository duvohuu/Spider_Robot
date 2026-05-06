#include "pca9685.h"
#include "board_config.h"
#include "utils.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_NUM  I2C_NUM_0

#define MODE1      0x00
#define PRESCALE   0xFE
#define LED0_ON_L  0x06

void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_MASTER_SDA,
        .scl_io_num       = I2C_MASTER_SCL,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

static void pca_write(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    i2c_master_write_to_device(I2C_MASTER_NUM, addr, buf, 2,
                               pdMS_TO_TICKS(10));
}

void pca_init(uint8_t addr)
{
    pca_write(addr, MODE1, 0x10);       // sleep
    pca_write(addr, PRESCALE, 121);     // ~50 Hz
    pca_write(addr, MODE1, 0x00);       // wake
    vTaskDelay(pdMS_TO_TICKS(5));
    pca_write(addr, MODE1, 0xA1);       // auto-increment + restart
}

static void pca_set_pwm(uint8_t addr, uint8_t ch, uint16_t off)
{
    uint8_t reg = LED0_ON_L + 4 * ch;
    uint8_t d[5] = { reg, 0, 0, off & 0xFF, off >> 8 };
    i2c_master_write_to_device(I2C_MASTER_NUM, addr, d, 5,
                               pdMS_TO_TICKS(10));
}

// pulse: 102 tick = 0° (0.5ms), 512 tick = 180° (2.5ms) tại 50Hz/4096
void servo_angle(uint8_t addr, uint8_t ch, float deg)
{
    deg = CLAMP(deg, 0.0f, 180.0f);
    uint16_t pulse = (uint16_t)(102.0f + (deg / 180.0f) * 410.0f);
    pca_set_pwm(addr, ch, pulse);
}