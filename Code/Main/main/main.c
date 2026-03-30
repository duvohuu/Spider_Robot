#include <stdio.h>
#include <math.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA 21
#define I2C_MASTER_SCL 22
#define I2C_FREQ 100000

#define PCA9685_ADDR 0x40

#define MODE1 0x00
#define PRESCALE 0xFE
#define LED0_ON_L 0x06

// ---------------- I2C INIT ----------------
void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// ---------------- I2C WRITE ----------------
void pca9685_write(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};

    i2c_master_write_to_device(
        I2C_MASTER_NUM,
        PCA9685_ADDR,
        buf,
        2,
        pdMS_TO_TICKS(100)
    );
}

// ---------------- PCA9685 INIT ----------------
void pca9685_init()
{
    pca9685_write(MODE1, 0x10);   // sleep

    pca9685_write(PRESCALE, 121); // 50Hz servo

    pca9685_write(MODE1, 0x00);   // wake
    vTaskDelay(pdMS_TO_TICKS(10));

    pca9685_write(MODE1, 0xA1);   // auto increment
}

// ---------------- SET PWM ----------------
void pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off)
{
    uint8_t reg = LED0_ON_L + 4 * channel;

    uint8_t data[5];

    data[0] = reg;
    data[1] = on & 0xFF;
    data[2] = on >> 8;
    data[3] = off & 0xFF;
    data[4] = off >> 8;

    i2c_master_write_to_device(
        I2C_MASTER_NUM,
        PCA9685_ADDR,
        data,
        5,
        pdMS_TO_TICKS(100)
    );
}

// ---------------- SERVO ANGLE ----------------
void servo_angle(uint8_t ch, float angle)
{
    float pulse = 205 + (angle / 180.0) * (410 - 205);
    pca9685_set_pwm(ch, 0, (uint16_t)pulse);
}

// ---------------- MAIN ----------------
void app_main()
{
    i2c_master_init();
    pca9685_init();

    while (1)
    {
        printf("0 degree\n");
        servo_angle(0, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("90 degree\n");
        servo_angle(0, 90);
        vTaskDelay(pdMS_TO_TICKS(2000));

        printf("180 degree\n");
        servo_angle(0, 180);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}