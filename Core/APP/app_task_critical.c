/**
  ******************************************************************************
  * @file    app_task_critical.c
  * @brief   临界区演示任务 - 展示taskENTER_CRITICAL()对不同优先级中断的影响
  * 
  * @note    实验原理：
  *          configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5
  *          
  *          TIM3优先级 = 6 (数值大，优先级低) >= 5
  *            -> 会被taskENTER_CRITICAL()阻止
  *          
  *          TIM4优先级 = 4 (数值小，优先级高) < 5  
  *            -> 不会被taskENTER_CRITICAL()阻止
  *          
  *          进入临界区时：
  *            - TIM3计数停止增长（中断被屏蔽）
  *            - TIM4计数继续增长（中断未被屏蔽）
  ******************************************************************************
  */

#include "app_task_critical.h"
#include "bsp_tim.h"
#include "bsp_led.h"
#include "bsp_oled.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

osThreadId_t criticalTaskHandle;

/* 静态任务资源 */
static StaticTask_t criticalTaskTCB;
static StackType_t  criticalTaskStack[CRITICAL_TASK_STACK_SIZE];

const osThreadAttr_t criticalTask_attr = {
    .name = CRITICAL_TASK_NAME,
    .cb_mem = &criticalTaskTCB,
    .cb_size = sizeof(criticalTaskTCB),
    .stack_mem = &criticalTaskStack[0],
    .stack_size = sizeof(criticalTaskStack),
    .priority = CRITICAL_TASK_PRIORITY,
};

/* 简单的串口打印函数 */
static void UART_Print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

static void UART_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    UART_Print(buf);
}

/**
  * @brief  临界区演示任务
  * @param  argument: 未使用
  * @note   循环执行：正常状态 -> 临界区状态 -> 正常状态
  *         LED0亮=临界区, LED0灭=正常
  *         LED1每次循环翻转，指示程序运行
  */
void Task_Critical(void *argument)
{
    (void)argument;
    
    uint32_t tim3_start, tim3_end;
    uint32_t tim4_start, tim4_end;
    uint32_t cycle = 0;
    
    /* OLED显示标题 */
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(1, 1, "Critical Demo");
    OLED_ShowString(2, 1, "MAX_PRIO=5");
    
    /* 串口输出标题 */
    UART_Print("\r\n\r\n");
    UART_Print("========================================\r\n");
    UART_Print("    FreeRTOS Critical Section Demo\r\n");
    UART_Print("========================================\r\n");
    UART_Print("configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5\r\n");
    UART_Print("TIM3 Priority: 6 (Will be BLOCKED)\r\n");
    UART_Print("TIM4 Priority: 4 (Will NOT be blocked)\r\n");
    UART_Print("----------------------------------------\r\n\r\n");
    
    for(;;)
    {
        cycle++;
        BSP_LED1_Toggle();  /* LED1翻转表示程序在运行 */
        
        /* ========== 正常状态 2秒 ========== */
        BSP_LED0_Off();  /* LED0灭表示正常 */
        OLED_ShowString(3, 1, "NORMAL        ");
        
        tim3_start = tim3_count;
        tim4_start = tim4_count;
        
        osDelay(2000);  /* 正常运行2秒 */
        
        tim3_end = tim3_count;
        tim4_end = tim4_count;
        
        /* OLED显示正常状态结果 (行4显示TIM3/TIM4计数) */
        OLED_ShowString(4, 1, "N:");
        OLED_ShowNum(4, 3, tim3_end - tim3_start, 4);
        OLED_ShowString(4, 8, "/");
        OLED_ShowNum(4, 9, tim4_end - tim4_start, 4);
        
        UART_Printf("[Cycle %lu] NORMAL 2s:\r\n", cycle);
        UART_Printf("  TIM3 (P6): delta = %lu\r\n", tim3_end - tim3_start);
        UART_Printf("  TIM4 (P4): delta = %lu\r\n", tim4_end - tim4_start);
        
        /* ========== 进入临界区 1秒 ========== */
        BSP_LED0_On();   /* LED0亮表示临界区 */
        OLED_ShowString(3, 1, "CRITICAL      ");
        
        tim3_start = tim3_count;
        tim4_start = tim4_count;
        
        /* 进入临界区 */
        taskENTER_CRITICAL();
        
        /* 软件延时约1秒 */
        for(volatile uint32_t i = 0; i < 16800000; i++) {
            __NOP();
        }
        
        /* 离开临界区 */
        taskEXIT_CRITICAL();
        
        tim3_end = tim3_count;
        tim4_end = tim4_count;
        
        BSP_LED0_Off();
        
        /* OLED显示临界区结果 (C: TIM3/TIM4) */
        OLED_ShowString(3, 1, "NORMAL        ");
        OLED_ShowString(4, 1, "C:");
        OLED_ShowNum(4, 3, tim3_end - tim3_start, 4);
        OLED_ShowString(4, 8, "/");
        OLED_ShowNum(4, 9, tim4_end - tim4_start, 4);
        
        UART_Printf("[Cycle %lu] CRITICAL 1s:\r\n", cycle);
        UART_Printf("  TIM3 (P6): delta = %lu (blocked!)\r\n", tim3_end - tim3_start);
        UART_Printf("  TIM4 (P4): delta = %lu\r\n", tim4_end - tim4_start);
        UART_Print("----------------------------------------\r\n");
        
        osDelay(500);
    }
}
