//
// Created by dxxdx on 2026/11/23.
//

#include "GPIO.h"

#include "ch32v20x.h"

/* 按板上网表配置时钟与引脚：旋钮 PA5~7、LED PB0、蜂鸣器 PB1、OLED PB6/7。
 * SystemInit 须先将 HCLK 配到 144 MHz；本函数把 APB1 分到 36 MHz。 */
void BSP_GPIO_Init()
{

    /* HCLK 144 MHz；APB1 /4 = 36 MHz。保留 CFGR0 其他位。 */
    RCC->CFGR0 = (RCC->CFGR0 & 0xFFFFF8FFu) | 0x00000500u;

    /* 先只开 GPIOA/B 和 TIM2/3；I2C1、DMA1 留给各自的初始化函数。 */
    RCC->APB2PCENR |= 0x0000000Cu;
    RCC->APB1PCENR |= 0x00000003u;

    /* 先预置电平：PA5~7 上拉；PB6/7 高，PB0/1 低。 */
    GPIOA->BSHR = 0x000000E0u;
    GPIOB->BSHR = 0x000300C0u;

    /* PA5~7：上拉输入；PA0~4：浮空输入。 */
    GPIOA->CFGLR = 0x88844444u;

    /* PB7/6：复用开漏；PB1：复用推挽；PB0：推挽输出。
       PB2~5：浮空输入。 */
    GPIOB->CFGLR = 0xDD4444A2u;

}

