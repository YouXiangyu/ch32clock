#include "IRQ.h"
#include "ch32v20x.h"
#include "clocktick.h"
#include "Component/EC11/EC11.h"

/* A 相先于 B 相闭合为正向：11→01→00→10→11。 */
static const int8_t ec11QuadratureDelta[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0,
};
static uint8_t ec11PreviousAB;
static int8_t ec11QuarterSteps = 0;

enum {
    EC11_NO_EVENT,
    EC11_FORWARD,
    EC11_REVERSE,
    EC11_PRESS,
};

/* 配置 DMA1_CH6、I2C1 事件/错误和旋钮 EXTI5~7 的中断优先级与使能。
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

    /* 在放行 EXTI 前采样 A/B 初态；首次完整相位变化才产生转动事件。 */
    ec11PreviousAB = (uint8_t)((GPIOA->INDR >> 6U) & 0x03u);

    PFIC->IPRIOR[32] = 0x40u; /* DMA1 Channel6。 */
    PFIC->IPRIOR[39] = 0x20u; /* EXTI9_5。 */
    PFIC->IPRIOR[47] = 0x40u; /* I2C1 事件：SB、ADDR、BTF。 */
    PFIC->IPRIOR[48] = 0x20u; /* I2C1 错误：含 NACK。 */
    PFIC->IPRR[1] = 0x00018081u; /* 清四路 pending。 */
    PFIC->IENR[1] = 0x00018081u; /* 使能四路 IRQ。 */

    EXTI->INTENR |= 0x000000E0u;
}

/* PA5 按下或 PA6/PA7 完成一格转动时，在当前 5 ms 节拍内最多计一次事件。 */
void EXTI9_5_IRQHandler(void)
    __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI9_5_IRQHandler(void)
{
    /* INTFR 写 1 清除；读取当前引脚电平解码本次边沿。 */
    uint32_t pending = EXTI->INTFR & 0x000000E0u;
    EXTI->INTFR = pending;
    uint32_t pins = GPIOA->INDR;
    uint8_t event = EC11_NO_EVENT;

    if ((pending & 0x000000C0u) != 0U) {
        uint8_t ab = (uint8_t)((pins >> 6U) & 0x03u); /* bit1=A(PA7)，bit0=B(PA6)。 */
        if ((ec11PreviousAB ^ ab) == 3U) {
            ec11QuarterSteps = 0; /* 两相同时跳变，丢弃不完整的一格。 */
        } else {
            ec11QuarterSteps += ec11QuadratureDelta[(ec11PreviousAB << 2U) | ab];
        }
        ec11PreviousAB = ab;

        /* 00/11 为定位点；两次有效相位变化记一格。 */
        if (ab == 0U || ab == 3U) {
            if (ec11QuarterSteps >= 2) event = EC11_FORWARD;
            else if (ec11QuarterSteps <= -2) event = EC11_REVERSE;
            ec11QuarterSteps = 0;
        }
    }

    /* PA5 只开下降沿；与旋转同时到来时优先记录按下。 */
    if ((pending & 0x00000020u) != 0U && (pins & 0x00000020u) == 0U) {
        event = EC11_PRESS;
    }
    if (event == EC11_NO_EVENT) return;

    uint32_t tick = clockTick5ms;
    if (g_ec11.hasLastEventTick != 0U && g_ec11.lastEventTick5ms == tick) return;
    g_ec11.lastEventTick5ms = tick;
    g_ec11.hasLastEventTick = 1U;

    /* 饱和计数，主循环来不及取走时不回绕到 0。 */
    if (event == EC11_FORWARD && g_ec11.forwardCount != UINT8_MAX) ++g_ec11.forwardCount;
    else if (event == EC11_REVERSE && g_ec11.reverseCount != UINT8_MAX) ++g_ec11.reverseCount;
    else if (event == EC11_PRESS && g_ec11.pressCount != UINT8_MAX) ++g_ec11.pressCount;
}
