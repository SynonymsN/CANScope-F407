/**
  ******************************************************************************
  * @file    bsp_tim.c
  * @brief   定时器驱动 - 演示临界区对不同优先级中断的影响
  * @note    TIM3: 优先级6 (>=5, 被临界区屏蔽)
  *          TIM4: 优先级4 (<5, 不被临界区屏蔽)
  ******************************************************************************
  */

#include "bsp_tim.h"

/* 定时器句柄 */
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

/* 中断计数器 */
volatile uint32_t tim3_count = 0;  /* 优先级6 - 受FreeRTOS管辖 */
volatile uint32_t tim4_count = 0;  /* 优先级4 - 不受FreeRTOS管辖 */

/**
  * @brief  TIM3初始化 - 优先级6（百姓，受FreeRTOS管辖）
  * @note   1ms中断一次
  */
void BSP_TIM3_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    /* APB1定时器时钟 = 84MHz (假设SYSCLK=168MHz, APB1分频4, 定时器x2) */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 8400 - 1;        /* 84MHz / 8400 = 10kHz */
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 10 - 1;             /* 10kHz / 10 = 1kHz = 1ms */
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim3);
    
    /* 配置NVIC - 优先级6 (会被临界区屏蔽) */
    HAL_NVIC_SetPriority(TIM3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

/**
  * @brief  TIM4初始化 - 优先级4（皇帝，不受FreeRTOS管辖）
  * @note   1ms中断一次
  */
void BSP_TIM4_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();
    
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 8400 - 1;        /* 84MHz / 8400 = 10kHz */
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 10 - 1;             /* 10kHz / 10 = 1kHz = 1ms */
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim4);
    
    /* 配置NVIC - 优先级4 (不会被临界区屏蔽) */
    HAL_NVIC_SetPriority(TIM4_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

/**
  * @brief  启动两个定时器
  */
void BSP_TIM_Start(void)
{
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start_IT(&htim4);
}
