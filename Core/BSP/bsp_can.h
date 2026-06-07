/**
  ******************************************************************************
  * @file    bsp_can.h
  * @brief   CAN1 驱动头文件
  ******************************************************************************
  */

#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

typedef struct
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];
} BSP_CAN_RxFrame_t;

typedef void (*BSP_CAN_RxCallback_t)(const BSP_CAN_RxFrame_t *frame);

/* 全局接收缓存 */
extern CAN_RxHeaderTypeDef g_rx_header;
extern uint8_t  g_rx_data[8];
extern volatile uint8_t g_can_rx_flag;
extern volatile uint8_t g_new_data_flag; 
extern volatile uint8_t g_rx_speed;
extern volatile uint8_t g_rx_light;
extern volatile uint8_t g_rx_lamp;

/* 函数声明 */
void    BSP_CAN_Filter_Start(void);
void    BSP_CAN1_FilterAndStart(void);
uint8_t BSP_CAN1_Send_Msg(uint32_t id, uint8_t *msg, uint8_t len);
void    BSP_CAN_RegisterRxCallback(BSP_CAN_RxCallback_t callback);
uint32_t BSP_CAN1_GetErrorCode(void);
uint32_t BSP_CAN1_GetTxFreeLevel(void);
uint32_t BSP_CAN1_GetTxPendingMask(void);
uint8_t BSP_CAN1_AbortTx(uint32_t tx_mailbox_mask);
uint8_t BSP_CAN1_Recover(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_CAN_H */
