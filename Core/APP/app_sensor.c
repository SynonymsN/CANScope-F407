#include "app_sensor.h"

#include <stdlib.h>

#include "cmsis_os2.h"
#include "i2c.h"

#define MPU6050_ADDR0       0x68U
#define MPU6050_ADDR1       0x69U
#define MPU6050_REG_PWR     0x6BU
#define MPU6050_REG_ACCEL_X 0x3BU
#define MPU6050_REG_WHOAMI  0x75U

typedef enum
{
    APP_SENSOR_NONE = 0,
    APP_SENSOR_MPU6050,
    APP_SENSOR_GENERIC_I2C,
} AppSensorType_t;

static AppSensorType_t s_type;
static uint8_t s_addr;
static uint16_t s_fallback_counter;

static bool i2c_ready(uint8_t addr)
{
    return HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 1U, 3U) == HAL_OK;
}

static bool i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1,
                            (uint16_t)(addr << 1),
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            buf,
                            len,
                            50U) == HAL_OK;
}

static bool i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1,
                             (uint16_t)(addr << 1),
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1U,
                             50U) == HAL_OK;
}

static bool detect_mpu6050(uint8_t addr)
{
    uint8_t who = 0U;

    if (!i2c_ready(addr)) {
        return false;
    }

    if (!i2c_read_reg(addr, MPU6050_REG_WHOAMI, &who, 1U)) {
        return false;
    }

    if ((who != 0x68U) && (who != 0x69U)) {
        return false;
    }

    (void)i2c_write_reg(addr, MPU6050_REG_PWR, 0x00U);
    return true;
}

void AppSensor_Init(void)
{
    s_type = APP_SENSOR_NONE;
    s_addr = 0U;
    s_fallback_counter = 0U;

    if (detect_mpu6050(MPU6050_ADDR0)) {
        s_type = APP_SENSOR_MPU6050;
        s_addr = MPU6050_ADDR0;
        return;
    }

    if (detect_mpu6050(MPU6050_ADDR1)) {
        s_type = APP_SENSOR_MPU6050;
        s_addr = MPU6050_ADDR1;
        return;
    }

    for (uint8_t addr = 0x08U; addr <= 0x77U; addr++) {
        if (i2c_ready(addr)) {
            s_type = APP_SENSOR_GENERIC_I2C;
            s_addr = addr;
            return;
        }
    }
}

void AppSensor_Poll(AppSensorSample_t *sample)
{
    if (sample == 0) {
        return;
    }

    sample->value = 0U;
    sample->address = s_addr;
    sample->flags = 0U;
    sample->valid = false;

    if (s_type == APP_SENSOR_MPU6050) {
        uint8_t data[2] = {0U, 0U};

        if (i2c_read_reg(s_addr, MPU6050_REG_ACCEL_X, data, sizeof(data))) {
            int16_t ax = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
            uint16_t mag = (uint16_t)((ax < 0) ? -ax : ax);

            sample->value = (uint16_t)(mag >> 4);
            sample->flags = APP_SENSOR_FLAG_PRESENT | APP_SENSOR_FLAG_MPU6050;
            sample->valid = true;
            return;
        }

        sample->flags = APP_SENSOR_FLAG_PRESENT | APP_SENSOR_FLAG_MPU6050 | APP_SENSOR_FLAG_ERROR;
        return;
    }

    if (s_type == APP_SENSOR_GENERIC_I2C) {
        s_fallback_counter++;
        sample->value = (uint16_t)(((uint16_t)s_addr << 4) | (s_fallback_counter & 0x0FU));
        sample->flags = APP_SENSOR_FLAG_PRESENT | APP_SENSOR_FLAG_GENERIC_I2C;
        sample->valid = true;
        return;
    }

    sample->flags = APP_SENSOR_FLAG_ERROR;
}

const char *AppSensor_Name(void)
{
    if (s_type == APP_SENSOR_MPU6050) {
        return "MPU6050";
    }

    if (s_type == APP_SENSOR_GENERIC_I2C) {
        return "I2C-DEV";
    }

    return "NO-I2C";
}
