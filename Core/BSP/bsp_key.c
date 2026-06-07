/**
  ******************************************************************************
  * @file    bsp_key.c
  * @brief   按键驱动实现 (BSP层 - 支持中断模式)
  ******************************************************************************
  */

#include "bsp_key.h"

/**
  * @brief  按键GPIO初始化 - 普通输入模式
  */
void BSP_KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    GPIO_InitStruct.Pin  = KEY0_PIN | KEY1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

/**
  * @brief  按键GPIO初始化 - 外部中断模式(下降沿触发)
  */
void BSP_KEY_EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    /* 配置为下降沿触发的外部中断 */
    GPIO_InitStruct.Pin  = KEY0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  /* 下降沿触发 */
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY0_PORT, &GPIO_InitStruct);
    
    /* 配置NVIC中断优先级 - 必须低于configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY(5) */
    HAL_NVIC_SetPriority(KEY0_EXTI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(KEY0_EXTI_IRQn);
}

/**
  * @brief  读取KEY0状态
  */
uint8_t BSP_KEY0_Read(void)
{
    if (HAL_GPIO_ReadPin(KEY0_PORT, KEY0_PIN) == GPIO_PIN_RESET)
    {
        return KEY_PRESSED;
    }
    return KEY_RELEASED;
}

uint8_t BSP_KEY1_Read(void)
{
    if (HAL_GPIO_ReadPin(KEY1_PORT, KEY1_PIN) == GPIO_PIN_RESET)
    {
        return KEY_PRESSED;
    }
    return KEY_RELEASED;
}
