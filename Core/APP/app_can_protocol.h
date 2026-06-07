#ifndef APP_CAN_PROTOCOL_H
#define APP_CAN_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "bsp_can.h"

#define APP_CAN_ID_HEARTBEAT       0x100U
#define APP_CAN_ID_VEHICLE_STATUS  0x123U
#define APP_CAN_ID_TEST_TX         0x200U
#define APP_CAN_NODE_TIMEOUT_MS    1000U

typedef struct
{
    uint8_t speed_kmh;
    uint8_t light_percent;
    uint8_t lamp_on;
} AppCanVehicleStatus_t;

bool AppCanProtocol_IsHeartbeat(const BSP_CAN_RxFrame_t *frame);
bool AppCanProtocol_DecodeVehicleStatus(const BSP_CAN_RxFrame_t *frame,
                                        AppCanVehicleStatus_t *status);

#ifdef __cplusplus
}
#endif

#endif
