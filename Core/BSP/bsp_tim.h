/**
  ******************************************************************************
  * @file    bsp_tim.h
  * @brief   定时器驱动头文件 - TIM3(优先级6) TIM4(优先级4)
  ******************************************************************************
  */

#ifndef __BSP_TIM_H
#define __BSP_TIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* 定时器句柄 */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/* 中断计数器 - 用于演示临界区效果 */
extern volatile uint32_t tim3_count;  /* 优先级6 - 受FreeRTOS管辖 */
extern volatile uint32_t tim4_count;  /* 优先级4 - 不受FreeRTOS管辖 */

/* 函数声明 */
void BSP_TIM3_Init(void);   /* 优先级6: 百姓，会被临界区屏蔽 */
void BSP_TIM4_Init(void);   /* 优先级4: 皇帝，不会被临界区屏蔽 */
void BSP_TIM_Start(void);   /* 启动两个定时器 */

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TIM_H */
