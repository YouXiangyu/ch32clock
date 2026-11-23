//
// Created by dxxdx on 2026/11/23.
//

#ifndef CH32CLOCK_BOOTCFG_H
#define CH32CLOCK_BOOTCFG_H
#include <stdint.h>

/* 上电时 EC11 按键按下则置 1，供后续入口选择使用。 */
extern uint8_t goBootloader;

void checkBootloader(void);
#endif //CH32CLOCK_BOOTCFG_H
