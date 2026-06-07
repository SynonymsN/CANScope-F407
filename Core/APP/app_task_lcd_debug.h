/**
  ******************************************************************************
  * @file    app_task_lcd_debug.h
  * @brief   LCD调试任务头文件
  ******************************************************************************
  */

#ifndef __APP_TASK_LCD_DEBUG_H
#define __APP_TASK_LCD_DEBUG_H

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

extern osThreadId_t lcdDebugTaskHandle;
extern const osThreadAttr_t lcdDebugTask_attr;

void Task_LcdDebug(void *argument);

#endif /* __APP_TASK_LCD_DEBUG_H */
