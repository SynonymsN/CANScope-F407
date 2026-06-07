#ifndef APP_CAN_MONITOR_H
#define APP_CAN_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmsis_os.h"
#include "bsp_can.h"

int32_t AppCanMonitor_Init(void);
void AppCanMonitor_StartBus(void);
osStatus_t AppCanMonitor_WaitFrame(BSP_CAN_RxFrame_t *frame, uint32_t timeout);
uint32_t AppCanMonitor_GetDropCount(void);
uint8_t AppCanMonitor_SendFrame(uint32_t std_id, uint8_t *payload, uint8_t len,
                                BSP_CAN_RxFrame_t *tx_frame);
uint8_t AppCanMonitor_SendTestFrame(uint8_t *payload, uint8_t len,
                                    BSP_CAN_RxFrame_t *tx_frame);
void AppCanMonitor_ServiceBus(void);
uint32_t AppCanMonitor_GetBusError(void);
uint32_t AppCanMonitor_GetRecoverCount(void);
uint32_t AppCanMonitor_GetTxAbortCount(void);
uint32_t AppCanMonitor_GetTxPendingCount(void);
uint8_t AppCanMonitor_IsBusOff(void);

#ifdef __cplusplus
}
#endif

#endif
