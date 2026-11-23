//
// Created by dxxdx on 2026/11/23.
//

#ifndef CH32CLOCK_CLOCKTICK_H
#define CH32CLOCK_CLOCKTICK_H
#include <stdint.h>

/* SysTick 每 5 ms 加 1；在中断中写、在主循环中读。 */
extern volatile uint32_t clockTick5ms;

/* 配置 SysTick 为 5 ms 周期，并启动计数。须在系统时钟和 PFIC 配好后调用。 */
void clockTick5ms_Init(void);

#endif //CH32CLOCK_CLOCKTICK_H
