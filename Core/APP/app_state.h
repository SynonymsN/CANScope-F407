#ifndef APP_STATE_H
#define APP_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "app_can_protocol.h"
#include "app_sensor.h"
#include "app_uart_protocol.h"
#include "bsp_can.h"

typedef struct
{
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t rx_queue_drops;
    uint32_t last_rx_tick;
    uint32_t last_tx_tick;
    uint32_t last_heartbeat_tick;
    uint32_t last_status_tick;
    uint32_t can_error_code;
    uint32_t can_recover_count;
    uint32_t can_tx_abort_count;
    uint32_t can_tx_pending_count;
    uint32_t sensor_sample_count;
    uint32_t uart_cmd_count;
    uint32_t uart_rx_bytes;
    uint32_t uart_frame_errors;
    uint32_t uart_checksum_errors;
    uint32_t uart_rx_overflows;
    uint16_t sensor_value;
    uint16_t sample_period_ms;
    uint16_t threshold;
    uint8_t sensor_flags;
    uint8_t sensor_address;
    uint8_t can_auto_enabled;

    bool has_rx_frame;
    bool has_tx_frame;
    bool has_vehicle_status;
    bool has_sensor_sample;
    bool sensor_valid;
    bool alarm_active;
    bool node_online;
    bool bus_off;

    BSP_CAN_RxFrame_t last_rx_frame;
    BSP_CAN_RxFrame_t last_tx_frame;
    AppCanVehicleStatus_t vehicle;
} AppStateSnapshot_t;

void AppState_Init(void);
void AppState_Reset(void);
void AppState_RecordRxFrame(const BSP_CAN_RxFrame_t *frame, uint32_t tick);
void AppState_RecordTxFrame(const BSP_CAN_RxFrame_t *frame, uint32_t tick);
void AppState_RecordHeartbeat(uint32_t tick);
void AppState_RecordVehicleStatus(const AppCanVehicleStatus_t *status, uint32_t tick);
void AppState_SetRxQueueDrops(uint32_t drops);
void AppState_SetCanDiag(uint32_t error_code, uint32_t recover_count, bool bus_off);
void AppState_SetCanTxDiag(uint32_t tx_abort_count, uint32_t tx_pending_count);
void AppState_RecordSensorSample(const AppSensorSample_t *sample, const AppRuntimeConfig_t *config);
void AppState_SetUartDiag(const AppRuntimeConfig_t *config);
void AppState_UpdateDiag(uint32_t tick);
void AppState_GetSnapshot(AppStateSnapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
