/**
  ******************************************************************************
  * @file    app_task_key.h
  * @brief   按键任务头文件 (App层 - 调用BSP和RTOS API)
  ******************************************************************************
  */

#ifndef __APP_TASK_KEY_H
#define __APP_TASK_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* 任务句柄声明 (extern) */
extern osThreadId_t task1Handle;
extern osThreadId_t task4Handle;

/* 任务属性声明 (extern) */
extern const osThreadAttr_t task1_attributes;
extern const osThreadAttr_t task4_attributes;

/* 任务函数声明 */
void Task1_CreateTasks(void *argument);
void Task4_KeyScan(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TASK_KEY_H */
