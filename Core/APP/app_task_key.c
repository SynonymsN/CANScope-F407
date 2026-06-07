/**
  ******************************************************************************
  * @file    app_task_key.c
  * @brief   按键任务实现 (App层 - 业务逻辑) - 静态任务创建版本
  ******************************************************************************
  */

#include "app_task_key.h"
#include "app_task_led.h"
#include "bsp_key.h"
#include "bsp_led.h"

/* 任务句柄定义 */
osThreadId_t task1Handle = NULL;
osThreadId_t task4Handle = NULL;

/* ============== 静态任务资源定义 ============== */
/* Task1 静态栈和控制块 */
static uint32_t task1Stack[128];
static StaticTask_t task1ControlBlock;

/* Task4 静态栈和控制块 */
static uint32_t task4Stack[128];
static StaticTask_t task4ControlBlock;

/* 任务属性定义 - 指定静态内存 */
const osThreadAttr_t task1_attributes = {
    .name = "Task1_Create",
    .cb_mem = &task1ControlBlock,
    .cb_size = sizeof(task1ControlBlock),
    .stack_mem = task1Stack,
    .stack_size = sizeof(task1Stack),
    .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t task4_attributes = {
    .name = "Task4_Key",
    .cb_mem = &task4ControlBlock,
    .cb_size = sizeof(task4ControlBlock),
    .stack_mem = task4Stack,
    .stack_size = sizeof(task4Stack),
    .priority = (osPriority_t) osPriorityNormal,
};

/**
  * @brief  Task1: 创建其他三个任务
  * @note   创建完成后进入空循环，等待被Task4删除
  * @param  argument: Not used
  */
void Task1_CreateTasks(void *argument)
{
    (void)argument;
    
    /* 创建Task2 - LED0闪烁任务 */
    task2Handle = osThreadNew(Task2_LED0_Blink, NULL, &task2_attributes);
    
    /* 创建Task3 - LED1闪烁任务 */
    task3Handle = osThreadNew(Task3_LED1_Blink, NULL, &task3_attributes);
    
    /* 创建Task4 - 按键扫描任务 */
    task4Handle = osThreadNew(Task4_KeyScan, NULL, &task4_attributes);
    
    /* Task1任务完成，进入空循环等待被删除 */
    for(;;)
    {
        osDelay(1000);
    }
}

/**
  * @brief  Task4: 按键扫描，按下KEY0挂起/恢复绿灯(LED1)任务
  * @note   第一次按下挂起绿灯，再次按下恢复绿灯
  * @param  argument: Not used
  */
void Task4_KeyScan(void *argument)
{
    (void)argument;
    uint8_t task3Suspended = 0;  /* 0: 运行中, 1: 已挂起 */
    
    for(;;)
    {
        if(BSP_KEY0_Read() == KEY_PRESSED)
        {
            osDelay(20);  /* 消抖 */
            if(BSP_KEY0_Read() == KEY_PRESSED)
            {
                if(task3Handle != NULL)
                {
                    if(task3Suspended == 0)
                    {
                        /* 挂起Task3 - 绿灯停止闪烁 */
                        osThreadSuspend(task3Handle);
                        BSP_LED1_Off();  /* 熄灭绿灯 */
                        task3Suspended = 1;
                    }
                    else
                    {
                        /* 恢复Task3 - 绿灯继续闪烁 */
                        osThreadResume(task3Handle);
                        task3Suspended = 0;
                    }
                }
                
                /* 等待按键释放 */
                while(BSP_KEY0_Read() == KEY_PRESSED)
                {
                    osDelay(10);
                }
            }
        }
        osDelay(10);
    }
}

