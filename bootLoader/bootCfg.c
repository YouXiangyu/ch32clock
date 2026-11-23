//
// Created by dxxdx on 2026/11/23.
//

#include "bootCfg.h"
#include "ch32v20x.h"

uint8_t goBootloader = 0u;

/* PA5 是上拉输入；EC11 按下时接地，为低电平。只锁存按下状态。 */
void checkBootloader(void)
{
    if ((GPIOA->INDR & 0x00000020u) == 0u) {
        goBootloader = 1u;
    }
}
