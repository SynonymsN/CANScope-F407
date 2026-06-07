/**
  ******************************************************************************
  * @file    app_task_critical.h
  * @brief   临界区演示任务头文件
  ******************************************************************************
  */

#ifndef __APP_TASK_CRITICAL_H
#define __APP_TASK_CRITICAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"

/* 任务句柄 */
extern osThreadId_t criticalTaskHandle;

/* 任务属性 */
extern const osThreadAttr_t criticalTask_attr;

/* 任务配置 */
#define CRITICAL_TASK_NAME        "CriticalTask"
#define CRITICAL_TASK_STACK_SIZE  512
#define CRITICAL_TASK_PRIORITY    osPriorityNormal

/* 临界区持续时间(ms) */
#define CRITICAL_SECTION_DURATION 1000

/* 任务函数 */
void Task_Critical(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TASK_CRITICAL_H */
