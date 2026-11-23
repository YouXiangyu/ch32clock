//
// Created by dxxdx on 2026/11/23.
//

#include "systemInit.h"
#include <stdint.h>
#include "system_ch32v20x.h"
#include "GPIO.h"
#include "I2C.h"
#include "IRQ.h"
#include "clocktick.h"

/* 板级初始化入口：系统时钟 → GPIO/总线 → I2C DMA → 中断 → 5 ms 时基。
 * 启动汇编直接进入 main，故 SystemInit 在这里调用一次即可。 */
void BSP_SystemInit(void)
{
    SystemInit();          /* HSE 8 MHz → 系统时钟 144 MHz */
    BSP_GPIO_Init();       /* 总线时钟、引脚，APB1 分频到 36 MHz */
    BSP_DMA_I2C_Init();    /* I2C1 400 kHz、DMA1 Channel6 */
    BSP_IRQ_Init();        /* DMA、I2C、PA5/6/7 和 SysTick 的中断 */
    clockTick5ms_Init();   /* SysTick 每 5 ms 中断一次 */
}




