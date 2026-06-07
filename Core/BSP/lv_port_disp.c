/**
 * @file lv_port_disp.c
 * @brief LVGL显示接口实现 - 适配正点原子探索者F407 FSMC LCD
 *
 * 使用单缓冲策略（10行），通过FSMC直接写入LCD显存。
 * 支持 ILI9341/ST7789 (240x320) 和 NT35310 (320x480)。
 */

#include "lv_port_disp.h"
#include "bsp_lcd.h"

/* 显示缓冲区 - 10行大小，按最大支持宽度480(ILI9806)分配 */
#define DISP_BUF_LINES  10
static lv_disp_draw_buf_t draw_buf_dsc;
static lv_color_t draw_buf[480 * DISP_BUF_LINES];

static lv_disp_drv_t disp_drv;

/**
 * @brief  LVGL显示刷新回调 - 将缓冲区数据写入LCD
 */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    (void)drv;

    /* 设置LCD窗口 */
    LCD->LCD_REG = lcddev.setxcmd;
    LCD->LCD_RAM = (uint16_t)(area->x1 >> 8);
    LCD->LCD_RAM = (uint16_t)(area->x1 & 0xFF);
    LCD->LCD_RAM = (uint16_t)(area->x2 >> 8);
    LCD->LCD_RAM = (uint16_t)(area->x2 & 0xFF);

    LCD->LCD_REG = lcddev.setycmd;
    LCD->LCD_RAM = (uint16_t)(area->y1 >> 8);
    LCD->LCD_RAM = (uint16_t)(area->y1 & 0xFF);
    LCD->LCD_RAM = (uint16_t)(area->y2 >> 8);
    LCD->LCD_RAM = (uint16_t)(area->y2 & 0xFF);

    /* 写GRAM命令 */
    LCD->LCD_REG = lcddev.wramcmd;

    /* 逐像素写入FSMC */
    uint32_t total = (uint32_t)(area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    for(uint32_t i = 0; i < total; i++) {
        LCD->LCD_RAM = color_p->full;
        color_p++;
    }

    lv_disp_flush_ready(drv);
}

/**
 * @brief  LVGL显示接口初始化
 * @note   必须在 BSP_LCD_Init() 之后调用
 */
void lv_port_disp_init(void)
{
    uint16_t hor_res = lcddev.width;
    uint16_t ver_res = lcddev.height;

    /* 缓冲区大小 = 屏幕宽度 x DISP_BUF_LINES */
    uint32_t buf_size = (uint32_t)hor_res * DISP_BUF_LINES;
    /* 安全限制：不超过静态数组大小 */
    if(buf_size > sizeof(draw_buf) / sizeof(draw_buf[0])) {
        buf_size = sizeof(draw_buf) / sizeof(draw_buf[0]);
    }

    lv_disp_draw_buf_init(&draw_buf_dsc, draw_buf, NULL, buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = hor_res;
    disp_drv.ver_res = ver_res;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc;

    lv_disp_drv_register(&disp_drv);
}
