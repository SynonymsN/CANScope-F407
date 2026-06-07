/**
  ******************************************************************************
  * @file    app_task_led.h
  * @brief   LED任务头文件 (App层 - 包含门铃任务)
  ******************************************************************************
  */

#ifndef __APP_TASK_LED_H
#define __APP_TASK_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* 任务句柄声明 (extern) */
extern osThreadId_t task2Handle;        /* 红灯LED0 */
extern osThreadId_t task3Handle;        /* 绿灯LED1 */
extern osThreadId_t doorbellTaskHandle; /* 门铃任务 */

/* 任务属性声明 (extern) */
extern const osThreadAttr_t task2_attributes;
extern const osThreadAttr_t task3_attributes;
extern const osThreadAttr_t doorbellTask_attributes;

/* 任务函数声明 */
void Task2_LED0_Blink(void *argument);   /* 红灯持续闪烁 */
void Task3_LED1_Blink(void *argument);   /* 绿灯持续闪烁 */
void Task_Doorbell(void *argument);      /* 门铃任务 - 绿灯闪3次 */

#ifdef __cplusplus
}
#endif

#endif /* __APP_TASK_LED_H */
