#ifndef APP_SENSOR_H
#define APP_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define APP_SENSOR_FLAG_PRESENT      0x01U
#define APP_SENSOR_FLAG_MPU6050      0x02U
#define APP_SENSOR_FLAG_GENERIC_I2C  0x04U
#define APP_SENSOR_FLAG_ERROR        0x80U

typedef struct
{
    uint16_t value;
    uint8_t address;
    uint8_t flags;
    bool valid;
} AppSensorSample_t;

void AppSensor_Init(void);
void AppSensor_Poll(AppSensorSample_t *sample);
const char *AppSensor_Name(void);

#ifdef __cplusplus
}
#endif

#endif
