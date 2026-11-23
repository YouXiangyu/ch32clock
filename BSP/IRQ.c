#include "IRQ.h"
#include "ch32v20x.h"

volatile uint32_t BSP_GPIO_IrqFlags;

/* 配置 SysTick、DMA1_CH6、I2C1 事件/错误和旋钮 EXTI5~7 的中断优先级与使能。
 * 调用前先完成 GPIO 和 I2C/DMA 初始化，避免在硬件未就绪时放行中断。 */
void BSP_IRQ_Init(void)
{
    /* PA5=旋钮按键，PA6/PA7=两相；EXTI5~7 共用 EXTI9_5 IRQ。 */
    RCC->APB2PCENR |= 0x00000001u; /* AFIO 时钟。 */
    EXTI->INTENR &= ~0x000000E0u;
    AFIO->EXTICR[1] &= 0xFFFF000Fu;

    /* 按键只捕获按下沿；旋钮两相捕获上升沿和下降沿。 */
    EXTI->RTENR = (EXTI->RTENR & ~0x000000E0u) | 0x000000C0u;
    EXTI->FTENR |= 0x000000E0u;
    EXTI->INTFR = 0x000000E0u;

    PFIC->IPRIOR[32] = 0x40u; /* DMA1 Channel6。 */
    PFIC->IPRIOR[39] = 0x20u; /* EXTI9_5。 */
    PFIC->IPRIOR[47] = 0x40u; /* I2C1 事件：SB、ADDR、BTF。 */
    PFIC->IPRIOR[48] = 0x20u; /* I2C1 错误：含 NACK。 */
    PFIC->IPRR[1] = 0x00018081u; /* 清四路 pending。 */
    PFIC->IENR[1] = 0x00018081u; /* 使能四路 IRQ。 */

    PFIC->IPRIOR[SysTick_IRQn] = 0x40u; /* 5 ms 时基。 */
    PFIC->IPRR[0] = 1u << SysTick_IRQn; /* 清 SysTick pending。 */
    PFIC->IENR[0] = 1u << SysTick_IRQn; /* 使能 SysTick IRQ。 */

    EXTI->INTENR |= 0x000000E0u;
}

/* 清 PA5~7 的 EXTI pending，并将发生过的边沿记入 BSP_GPIO_IrqFlags。
 * 标志是位集合，不记录边沿顺序；旋钮解码还需读取引脚状态。 */
void EXTI9_5_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI9_5_IRQHandler(void)
{
    /* INTFR 写 1 清除；标志只提示发生过边沿，不保留旋钮边沿顺序。 */
    uint32_t pending = EXTI->INTFR & 0x000000E0u;
    EXTI->INTFR = pending;
    BSP_GPIO_IrqFlags |= pending;
}
