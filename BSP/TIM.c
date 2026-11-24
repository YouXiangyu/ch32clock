//
// Created by dxxdx on 2026/11/24.
//

#include "TIM.h"
#include "ch32v20x.h"

#define TIM3_COUNTER_HZ 1000000u
#define TIM3_INPUT_HZ 72000000u /* HCLK=144 MHz，APB1=/4，TIM3 时钟=2×PCLK1。 */

static uint16_t tim_half_period(void)
{
    return (uint16_t)(((uint32_t)TIM3->ATRLR + 1u) / 2u);
}

/* 配置 PB1 为 TIM3_CH4 PWM1；先保持低电平，避免上电时蜂鸣器长鸣。 */
void TIM_Init(void)
{
    RCC->APB1PCENR |= 0x00000002u; /* TIM3 时钟。 */
    RCC->APB2PCENR |= 0x00000008u; /* GPIOB 时钟。 */
    GPIOB->BCR = 0x00000002u;      /* 预置 PB1 低电平。 */
    GPIOB->CFGLR = (GPIOB->CFGLR & ~0x000000F0u) | 0x00000020u;

    TIM3->CTLR1 = 0u;              /* 先停止计数。 */
    TIM3->CCER = 0u;
    TIM3->PSC = (uint16_t)(TIM3_INPUT_HZ / TIM3_COUNTER_HZ - 1u); /* 72 MHz → 1 MHz。 */
    TIM3->ATRLR = 999u;            /* 默认 1 kHz；启用前 CCR4=0，实际不发声。 */
    TIM3->CNT = 0u;
    TIM3->CHCTLR2 = 0x6000u;       /* CH4 PWM1，输出比较，关闭预装载。 */
    TIM3->CH4CVR = 0u;             /* PWM1 的 0% 占空比使 PB1 保持低电平。 */
    TIM3->CCER = 0x1000u;          /* CH4 输出使能，正极性。 */
    TIM3->SWEVGR = 0x0001u;        /* UG：装入 PSC，并从新周期开始。 */
    TIM3->INTFR = 0u;              /* 清 UG 产生的更新标志。 */
    TIM3->CTLR1 = 0x0001u;         /* 开始计数；输出仍为低电平。 */
    GPIOB->CFGLR = (GPIOB->CFGLR & ~0x000000F0u) | 0x000000A0u;
    /* 最后才把 PB1 交给定时器，避免配置期间出现高电平。 */
}

void TIM_SetAutoReload(uint32_t ar)
{
    if (ar < 1u) ar = 1u;
    if (ar > 0xFFFFu) ar = 0xFFFFu; /* TIM3 的 ARR 为 16 位。 */

    uint8_t wasEnabled = TIM3->CH4CVR != 0u;
    TIM3->CH4CVR = 0u;             /* 换频前先让输出变低。 */
    TIM3->CTLR1 &= (uint16_t)~0x0001u;
    TIM3->ATRLR = (uint16_t)ar;
    TIM3->CNT = 0u;
    TIM3->SWEVGR = 0x0001u;
    TIM3->INTFR = 0u;
    if (wasEnabled != 0u) TIM3->CH4CVR = tim_half_period();
    TIM3->CTLR1 |= 0x0001u;
}

void TIM_EnableOutput(void)
{
    TIM3->CNT = 0u;
    TIM3->CH4CVR = tim_half_period();
}

void TIM_DisableOutput(void)
{
    /* 保持 CH4 和计数器运行，CCR4=0 才能保证 PB1 被持续驱动为低。 */
    TIM3->CH4CVR = 0u;
}
