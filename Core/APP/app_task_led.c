/**
  ******************************************************************************
  * @file    app_task_led.c
  * @brief   LED任务实现 - 包含门铃任务(中断恢复演示)
  ******************************************************************************
  */

#include "app_task_led.h"
#include "bsp_led.h"

/* 任务句柄定义 */
osThreadId_t task2Handle = NULL;        /* 红灯LED0 */
osThreadId_t task3Handle = NULL;        /* 绿灯LED1 */
osThreadId_t doorbellTaskHandle = NULL; /* 门铃任务 */

/* ============== 静态任务资源定义 ============== */
/* Task2 静态栈和控制块 - 红灯 */
static uint32_t task2Stack[128];
static StaticTask_t task2ControlBlock;

/* Task3 静态栈和控制块 - 绿灯 */
static uint32_t task3Stack[128];
static StaticTask_t task3ControlBlock;

/* 门铃任务 静态栈和控制块 */
static uint32_t doorbellStack[128];
static StaticTask_t doorbellControlBlock;

/* 任务属性定义 */
const osThreadAttr_t task2_attributes = {
    .name = "Task2_LED0",
    .cb_mem = &task2ControlBlock,
    .cb_size = sizeof(task2ControlBlock),
    .stack_mem = task2Stack,
    .stack_size = sizeof(task2Stack),
    .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t task3_attributes = {
    .name = "Task3_LED1",
    .cb_mem = &task3ControlBlock,
    .cb_size = sizeof(task3ControlBlock),
    .stack_mem = task3Stack,
    .stack_size = sizeof(task3Stack),
    .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t doorbellTask_attributes = {
    .name = "Doorbell",
    .cb_mem = &doorbellControlBlock,
    .cb_size = sizeof(doorbellControlBlock),
    .stack_mem = doorbellStack,
    .stack_size = sizeof(doorbellStack),
    .priority = (osPriority_t) osPriorityAboveNormal,  /* 较高优先级 */
};

/**
  * @brief  Task2: 红灯(LED0)每500ms闪烁一次 - 持续闪烁
  */
void Task2_LED0_Blink(void *argument)
{
    (void)argument;
    
    for(;;)
    {
        BSP_LED0_Toggle();
        osDelay(500);
    }
}

/**
  * @brief  Task3: 绿灯(LED1)每500ms闪烁一次 - 持续闪烁
  */
void Task3_LED1_Blink(void *argument)
{
    (void)argument;
    
    for(;;)
    {
        BSP_LED1_Toggle();
        osDelay(500);
    }
}

/**
  * @brief  门铃任务 - 绿灯闪烁3次后自己挂起
  * @note   初始状态为挂起，由按键中断恢复
  *         每次恢复后闪烁3次(叮-咚-叮)，然后再次挂起自己
  */
void Task_Doorbell(void *argument)
{
    (void)argument;
    uint8_t i;
    
    for(;;)
    {
        /* 闪烁3次: 叮-咚-叮 */
        for(i = 0; i < 3; i++)
        {
            BSP_LED1_On();    /* 亮 */
            osDelay(200);
            BSP_LED1_Off();   /* 灭 */
            osDelay(200);
        }
        
        /* 完成后挂起自己，等待下次按键中断恢复 */
        osThreadSuspend(osThreadGetId());
    }
}
