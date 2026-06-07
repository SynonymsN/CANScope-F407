/**
  ******************************************************************************
  * @file    bsp_key.h
  * @brief   按键驱动头文件 (BSP层 - 支持中断模式)
  ******************************************************************************
  */

#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* 按键引脚定义 - 正点原子探索者F407 */
#define KEY0_PIN          GPIO_PIN_4
#define KEY0_PORT         GPIOE
 #define KEY1_PIN          GPIO_PIN_3
#define KEY1_PORT         GPIOE
#define KEY0_EXTI_IRQn    EXTI4_IRQn      /* PE4 对应 EXTI4 */

/* 按键状态定义 */
#define KEY_PRESSED       1
#define KEY_RELEASED      0

/* 函数声明 */
void BSP_KEY_Init(void);              /* 普通输入模式 */
void BSP_KEY_EXTI_Init(void);         /* 外部中断模式 */
uint8_t BSP_KEY0_Read(void);
uint8_t BSP_KEY1_Read(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_KEY_H */
