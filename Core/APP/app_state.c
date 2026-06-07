#include "app_state.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

static AppStateSnapshot_t s_state;

static void lock_state(void)
{
    taskENTER_CRITICAL();
}

static void unlock_state(void)
{
    taskEXIT_CRITICAL();
}

void AppState_Init(void)
{
    AppState_Reset();
}

void AppState_Reset(void)
{
    lock_state();
    memset(&s_state, 0, sizeof(s_state));
    unlock_state();
}

void AppState_RecordRxFrame(const BSP_CAN_RxFrame_t *frame, uint32_t tick)
{
    if (frame == 0) {
        return;
    }

    lock_state();
    s_state.rx_count++;
    s_state.last_rx_tick = tick;
    s_state.last_rx_frame = *frame;
    s_state.has_rx_frame = true;
    unlock_state();
}

void AppState_RecordTxFrame(const BSP_CAN_RxFrame_t *frame, uint32_t tick)
{
    if (frame == 0) {
        return;
    }

    lock_state();
    s_state.tx_count++;
    s_state.last_tx_tick = tick;
    s_state.last_tx_frame = *frame;
    s_state.has_tx_frame = true;
    unlock_state();
}

void AppState_RecordHeartbeat(uint32_t tick)
{
    lock_state();
    s_state.last_heartbeat_tick = tick;
    s_state.node_online = true;
    unlock_state();
}

void AppState_RecordVehicleStatus(const AppCanVehicleStatus_t *status, uint32_t tick)
{
    if (status == 0) {
        return;
    }

    lock_state();
    s_state.vehicle = *status;
    s_state.has_vehicle_status = true;
    s_state.last_status_tick = tick;
    s_state.node_online = true;
    unlock_state();
}

void AppState_SetRxQueueDrops(uint32_t drops)
{
    lock_state();
    s_state.rx_queue_drops = drops;
    unlock_state();
}

void AppState_SetCanDiag(uint32_t error_code, uint32_t recover_count, bool bus_off)
{
    lock_state();
    s_state.can_error_code = error_code;
    s_state.can_recover_count = recover_count;
    s_state.bus_off = bus_off;
    unlock_state();
}

void AppState_SetCanTxDiag(uint32_t tx_abort_count, uint32_t tx_pending_count)
{
    lock_state();
    s_state.can_tx_abort_count = tx_abort_count;
    s_state.can_tx_pending_count = tx_pending_count;
    unlock_state();
}

void AppState_RecordSensorSample(const AppSensorSample_t *sample, const AppRuntimeConfig_t *config)
{
    if ((sample == 0) || (config == 0)) {
        return;
    }

    lock_state();
    s_state.sensor_sample_count++;
    s_state.sensor_value = sample->value;
    s_state.sensor_flags = sample->flags;
    s_state.sensor_address = sample->address;
    s_state.sensor_valid = sample->valid;
    s_state.has_sensor_sample = true;
    s_state.sample_period_ms = config->sample_period_ms;
    s_state.threshold = config->threshold;
    s_state.can_auto_enabled = config->can_enabled;
    s_state.alarm_active = (sample->valid && (sample->value >= config->threshold));
    unlock_state();
}

void AppState_SetUartDiag(const AppRuntimeConfig_t *config)
{
    if (config == 0) {
        return;
    }

    lock_state();
    s_state.uart_cmd_count = config->cmd_count;
    s_state.uart_rx_bytes = config->rx_bytes;
    s_state.uart_frame_errors = config->frame_errors;
    s_state.uart_checksum_errors = config->checksum_errors;
    s_state.uart_rx_overflows = config->rx_overflows;
    s_state.sample_period_ms = config->sample_period_ms;
    s_state.threshold = config->threshold;
    s_state.can_auto_enabled = config->can_enabled;
    unlock_state();
}

void AppState_UpdateDiag(uint32_t tick)
{
    uint32_t last_tick;

    lock_state();
    last_tick = s_state.last_status_tick;
    if (s_state.last_heartbeat_tick > last_tick) {
        last_tick = s_state.last_heartbeat_tick;
    }

    if (last_tick == 0U) {
        s_state.node_online = false;
    } else {
        s_state.node_online =
            ((uint32_t)(tick - last_tick) <= APP_CAN_NODE_TIMEOUT_MS);
    }
    unlock_state();
}

void AppState_GetSnapshot(AppStateSnapshot_t *snapshot)
{
    if (snapshot == 0) {
        return;
    }

    lock_state();
    *snapshot = s_state;
    unlock_state();
}
