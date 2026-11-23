//
// Created by dxxdx on 2026/11/23.
//

#include "clocktick.h"
#include "ch32v20x.h"

volatile uint32_t clockTick5ms;

/* CH32V203 的 STK 是 32 位计数器；库头文件的 CNT/CMP 却声明为 64 位。
 * 直接访问手册中的 32 位比较低位寄存器，避免额外写入保留地址。 */
#define SYSTICK_CMPLR (*(volatile uint32_t *)(uintptr_t)0xE000F010u)

/* HCLK=144 MHz，SysTick 选 HCLK/8=18 MHz；90000 个周期为 5 ms。
 * 向上计数并自动从 0 重装，比较值设为 90000-1。 */
void clockTick5ms_Init(void)
{
    SysTick->CTLR = 0u;         /* 停止计数，关闭计数器中断。 */
    SYSTICK_CMPLR = 89999u;
    SysTick->CTLR = 0x20u;      /* INIT：把向上计数器置 0。 */
    SysTick->SR = 0u;           /* CNTIF 写 0 清除。 */
    clockTick5ms = 0u;
    SysTick->CTLR = 0x0Bu;      /* HCLK/8、向上计数、自动重装、中断和计数使能。 */
}

/* 向量表中的 SysTick IRQ：清比较标志并累计一个 5 ms 节拍。 */
void SysTick_Handler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void SysTick_Handler(void)
{
    if ((SysTick->SR & 0x01u) != 0u) {
        SysTick->SR = 0u;
        ++clockTick5ms;
    }
}
