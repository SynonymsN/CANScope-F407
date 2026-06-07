/**
  ******************************************************************************
  * @file    bsp_can.c
  * @brief   CAN1 驱动 — 过滤器 / 中断 / 发送 / 接收回调
  ******************************************************************************
  */

#include "bsp_can.h"
#include "can.h"
#include <string.h>

/* ============== 全局接收缓存 ============== */
CAN_RxHeaderTypeDef g_rx_header;
uint8_t  g_rx_data[8];
volatile uint8_t g_can_rx_flag = 0;
volatile uint8_t g_new_data_flag = 0;
volatile uint8_t g_rx_speed = 0;
volatile uint8_t g_rx_light = 0;
volatile uint8_t g_rx_lamp = 0;

static BSP_CAN_RxCallback_t s_rx_callback;
static volatile uint32_t s_irq_error_code;

void BSP_CAN_RegisterRxCallback(BSP_CAN_RxCallback_t callback)
{
    s_rx_callback = callback;
}

uint32_t BSP_CAN1_GetErrorCode(void)
{
    return HAL_CAN_GetError(&hcan1) | s_irq_error_code;
}

uint32_t BSP_CAN1_GetTxFreeLevel(void)
{
    return HAL_CAN_GetTxMailboxesFreeLevel(&hcan1);
}

uint32_t BSP_CAN1_GetTxPendingMask(void)
{
    uint32_t mask = 0U;

    if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX0) != 0U) {
        mask |= CAN_TX_MAILBOX0;
    }
    if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX1) != 0U) {
        mask |= CAN_TX_MAILBOX1;
    }
    if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX2) != 0U) {
        mask |= CAN_TX_MAILBOX2;
    }

    return mask;
}

uint8_t BSP_CAN1_AbortTx(uint32_t tx_mailbox_mask)
{
    if (tx_mailbox_mask == 0U) {
        return 0U;
    }

    return (HAL_CAN_AbortTxRequest(&hcan1, tx_mailbox_mask) == HAL_OK) ? 0U : 1U;
}

/**
  * @brief  配置过滤器并启动 CAN1（正常模式）
  * @note   当前配置为全通，可在回调中按 ID 做软件筛选
  */
void BSP_CAN_Filter_Start(void)
{
    CAN_FilterTypeDef sFilterConfig = {0};

    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1,
                                  CAN_IT_RX_FIFO0_MSG_PENDING |
                                  CAN_IT_RX_FIFO0_OVERRUN |
                                  CAN_IT_ERROR_WARNING |
                                  CAN_IT_ERROR_PASSIVE |
                                  CAN_IT_BUSOFF |
                                  CAN_IT_LAST_ERROR_CODE |
                                  CAN_IT_ERROR);
}

/**
  * @brief  初始化 CAN1：重置状态 + Init + 过滤器 + 启动 + 中断
  */
void BSP_CAN1_FilterAndStart(void)
{
    extern CAN_HandleTypeDef hcan1;

    /* 强制重置句柄状态，确保 Init 能执行 */
    __HAL_CAN_RESET_HANDLE_STATE(&hcan1);
    hcan1.Init.AutoRetransmission = DISABLE;
    HAL_CAN_Init(&hcan1);

    BSP_CAN_Filter_Start();
}

uint8_t BSP_CAN1_Recover(void)
{
    if (HAL_CAN_Stop(&hcan1) != HAL_OK) {
        __HAL_CAN_RESET_HANDLE_STATE(&hcan1);
    }

    hcan1.Init.AutoRetransmission = DISABLE;
    HAL_CAN_Init(&hcan1);
    BSP_CAN_Filter_Start();

    return (HAL_CAN_GetState(&hcan1) == HAL_CAN_STATE_ERROR) ? 1U : 0U;
}

/**
  * @brief  发送标准帧
  * @param  id   标准帧 ID (11-bit)
  * @param  msg  数据指针
  * @param  len  数据长度 (0-8)
  * @retval 0=成功, 1=失败
  */
uint8_t BSP_CAN1_Send_Msg(uint32_t id, uint8_t *msg, uint8_t len)
{
    CAN_TxHeaderTypeDef tx_hdr;
    uint32_t tx_mailbox;

    if ((msg == NULL) || (len > 8U)) {
        return 1;
    }

    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0U) {
        return 1;
    }

    tx_hdr.StdId = id;
    tx_hdr.ExtId = 0;
    tx_hdr.IDE   = CAN_ID_STD;
    tx_hdr.RTR   = CAN_RTR_DATA;
    tx_hdr.DLC   = len;
    tx_hdr.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(&hcan1, &tx_hdr, msg, &tx_mailbox) != HAL_OK)
    {
        return 1;
    }
    return 0;
}

/**
  * @brief  FIFO0 接收中断回调 — HAL 内部已清标志，此处只需读报文
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &g_rx_header, g_rx_data) == HAL_OK)
        {
            BSP_CAN_RxFrame_t frame;

            g_can_rx_flag = 1;

            frame.header = g_rx_header;
            memcpy(frame.data, g_rx_data, sizeof(frame.data));

            if ((g_rx_header.IDE == CAN_ID_STD) &&
                (g_rx_header.StdId == 0x123U) &&
                (g_rx_header.DLC >= 3U))
            {
                g_rx_speed = g_rx_data[0];
                g_rx_light = g_rx_data[1];
                g_rx_lamp  = g_rx_data[2];
                g_new_data_flag = 1;
            }

            if (s_rx_callback != NULL) {
                s_rx_callback(&frame);
            }
        }
    }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1) {
        s_irq_error_code |= HAL_CAN_GetError(hcan);
    }
}
