#include "app_can_monitor.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "app_can_protocol.h"

#define APP_CAN_RX_QUEUE_DEPTH  16U
#define APP_CAN_RECOVERY_PERIOD_MS 1000U
#define APP_CAN_TX_STALL_MS 300U

static osMessageQueueId_t s_rx_queue;
static StaticQueue_t s_rx_queue_cb;
static uint8_t s_rx_queue_mem[APP_CAN_RX_QUEUE_DEPTH * sizeof(BSP_CAN_RxFrame_t)];
static volatile uint32_t s_rx_queue_drops;
static volatile uint32_t s_bus_error;
static volatile uint32_t s_local_error_flags;
static volatile uint32_t s_recover_count;
static volatile uint32_t s_tx_abort_count;
static volatile uint32_t s_tx_pending_count;
static uint32_t s_last_recovery_tick;
static uint32_t s_tx_pending_since_tick;
static uint8_t s_bus_started;

static const osMessageQueueAttr_t s_rx_queue_attr = {
    .name = "CAN_RxQ",
    .cb_mem = &s_rx_queue_cb,
    .cb_size = sizeof(s_rx_queue_cb),
    .mq_mem = s_rx_queue_mem,
    .mq_size = sizeof(s_rx_queue_mem),
};

static void can_rx_from_isr(const BSP_CAN_RxFrame_t *frame)
{
    if ((s_rx_queue == 0) || (frame == 0)) {
        s_rx_queue_drops++;
        return;
    }

    if (osMessageQueuePut(s_rx_queue, frame, 0U, 0U) != osOK) {
        s_rx_queue_drops++;
    }
}

static uint32_t count_pending_mailboxes(uint32_t mask)
{
    uint32_t count = 0U;

    if ((mask & CAN_TX_MAILBOX0) != 0U) {
        count++;
    }
    if ((mask & CAN_TX_MAILBOX1) != 0U) {
        count++;
    }
    if ((mask & CAN_TX_MAILBOX2) != 0U) {
        count++;
    }

    return count;
}

static void service_tx_mailboxes(uint32_t now)
{
    uint32_t pending_mask = BSP_CAN1_GetTxPendingMask();

    s_tx_pending_count = count_pending_mailboxes(pending_mask);

    if (pending_mask == 0U) {
        s_tx_pending_since_tick = 0U;
        return;
    }

    if (s_tx_pending_since_tick == 0U) {
        s_tx_pending_since_tick = now;
        return;
    }

    if ((uint32_t)(now - s_tx_pending_since_tick) < APP_CAN_TX_STALL_MS) {
        return;
    }

    if (BSP_CAN1_AbortTx(pending_mask) == 0U) {
        s_tx_abort_count++;
        s_tx_pending_count = 0U;
        s_local_error_flags |= HAL_CAN_ERROR_ACK;
    }

    s_tx_pending_since_tick = 0U;
}

int32_t AppCanMonitor_Init(void)
{
    s_rx_queue = osMessageQueueNew(APP_CAN_RX_QUEUE_DEPTH,
                                   sizeof(BSP_CAN_RxFrame_t),
                                   &s_rx_queue_attr);
    if (s_rx_queue == 0) {
        return -1;
    }

    BSP_CAN_RegisterRxCallback(can_rx_from_isr);
    return 0;
}

void AppCanMonitor_StartBus(void)
{
    if (s_bus_started != 0U) {
        return;
    }

    BSP_CAN1_FilterAndStart();
    s_bus_started = 1U;
}

osStatus_t AppCanMonitor_WaitFrame(BSP_CAN_RxFrame_t *frame, uint32_t timeout)
{
    if ((s_rx_queue == 0) || (frame == 0)) {
        return osErrorParameter;
    }

    return osMessageQueueGet(s_rx_queue, frame, 0U, timeout);
}

uint32_t AppCanMonitor_GetDropCount(void)
{
    return s_rx_queue_drops;
}

uint8_t AppCanMonitor_SendFrame(uint32_t std_id, uint8_t *payload, uint8_t len,
                                BSP_CAN_RxFrame_t *tx_frame)
{
    uint8_t ret;
    uint32_t now;

    if ((payload == 0) || (len > 8U) || (std_id > 0x7FFU)) {
        return 1U;
    }

    now = osKernelGetTickCount();
    service_tx_mailboxes(now);

    if (BSP_CAN1_GetTxFreeLevel() == 0U) {
        return 1U;
    }

    if (tx_frame != 0) {
        memset(tx_frame, 0, sizeof(*tx_frame));
        tx_frame->header.StdId = std_id;
        tx_frame->header.IDE = CAN_ID_STD;
        tx_frame->header.RTR = CAN_RTR_DATA;
        tx_frame->header.DLC = len;
        memcpy(tx_frame->data, payload, len);
    }

    ret = BSP_CAN1_Send_Msg(std_id, payload, len);
    if (ret == 0U) {
        service_tx_mailboxes(now);
    }

    return ret;
}

void AppCanMonitor_ServiceBus(void)
{
    uint32_t now;
    uint32_t error;

    if (s_bus_started == 0U) {
        return;
    }

    service_tx_mailboxes(osKernelGetTickCount());

    error = BSP_CAN1_GetErrorCode() | s_local_error_flags;
    s_bus_error = error;

    if ((error & HAL_CAN_ERROR_BOF) == 0U) {
        return;
    }

    now = osKernelGetTickCount();
    if ((uint32_t)(now - s_last_recovery_tick) < APP_CAN_RECOVERY_PERIOD_MS) {
        return;
    }

    s_last_recovery_tick = now;
    if (BSP_CAN1_Recover() == 0U) {
        s_recover_count++;
        s_bus_error = BSP_CAN1_GetErrorCode();
    }
}

uint32_t AppCanMonitor_GetBusError(void)
{
    return s_bus_error;
}

uint32_t AppCanMonitor_GetRecoverCount(void)
{
    return s_recover_count;
}

uint32_t AppCanMonitor_GetTxAbortCount(void)
{
    return s_tx_abort_count;
}

uint32_t AppCanMonitor_GetTxPendingCount(void)
{
    return s_tx_pending_count;
}

uint8_t AppCanMonitor_IsBusOff(void)
{
    return ((s_bus_error & HAL_CAN_ERROR_BOF) != 0U) ? 1U : 0U;
}

uint8_t AppCanMonitor_SendTestFrame(uint8_t *payload, uint8_t len,
                                    BSP_CAN_RxFrame_t *tx_frame)
{
    return AppCanMonitor_SendFrame(APP_CAN_ID_TEST_TX, payload, len, tx_frame);
}
