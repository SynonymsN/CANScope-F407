/**
 ****************************************************************************************************
 * @file        bsp_oled.h
 * @brief       OLED驱动头文件 (SSD1306, I2C软件模拟)
 * @note        基于正点原子F407探索者开发板
 *              引脚: PB8(SCL), PB9(SDA)
 ****************************************************************************************************
 */

#ifndef __BSP_OLED_H
#define __BSP_OLED_H

#include "stm32f4xx_hal.h"

/******************************************************************************************/
/* I2C引脚定义 */

#define OLED_SCL_GPIO_PORT              GPIOB
#define OLED_SCL_GPIO_PIN               GPIO_PIN_8
#define OLED_SCL_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define OLED_SDA_GPIO_PORT              GPIOB
#define OLED_SDA_GPIO_PIN               GPIO_PIN_9
#define OLED_SDA_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

/******************************************************************************************/
/* IO操作宏 */

#define OLED_SCL(x)       do{ x ? \
                              HAL_GPIO_WritePin(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)

#define OLED_SDA(x)       do{ x ? \
                              HAL_GPIO_WritePin(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)

#define OLED_READ_SDA     HAL_GPIO_ReadPin(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN)

/******************************************************************************************/
/* OLED参数定义 */

#define OLED_I2C_ADDR               0x78    /* OLED I2C地址 */
#define OLED_WIDTH                  128     /* OLED宽度 */
#define OLED_HEIGHT                 64      /* OLED高度 */

/******************************************************************************************/
/* 函数声明 */

/* 初始化函数 */
void OLED_Init(void);

/* 基础显示函数 */
void OLED_Clear(void);
void OLED_SetCursor(uint8_t y, uint8_t x);
void OLED_WriteCommand(uint8_t cmd);
void OLED_WriteData(uint8_t data);

/* 字符显示函数 */
void OLED_ShowChar(uint8_t line, uint8_t column, char ch);
void OLED_ShowString(uint8_t line, uint8_t column, const char *str);
void OLED_ShowNum(uint8_t line, uint8_t column, uint32_t num, uint8_t len);
void OLED_ShowSignedNum(uint8_t line, uint8_t column, int32_t num, uint8_t len);
void OLED_ShowHexNum(uint8_t line, uint8_t column, uint32_t num, uint8_t len);

#endif /* __BSP_OLED_H */
