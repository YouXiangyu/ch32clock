//
// Created by dxxdx on 2026/11/23.
//

#ifndef CH32CLOCK_IRQ_H
#define CH32CLOCK_IRQ_H

#include <stdint.h>

void BSP_IRQ_Init(void);

/* EXTI 原始标志：bit5=按键，bit6=B 相，bit7=A 相。 */
extern volatile uint32_t BSP_GPIO_IrqFlags;

#endif
