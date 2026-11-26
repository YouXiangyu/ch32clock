//
// Created by dxxdx on 2026/11/24.
//

#ifndef CH32CLOCK_TIM_H
#define CH32CLOCK_TIM_H
#include <stdint.h>

/* 等十二平均律，枚举值是近似频率（Hz）。
 * OFF 表示静音，不能参与频率除法。 */
typedef enum BuzzerTone {
    BUZZER_TONE_OFF = 0,
    BUZZER_TONE_C4  = 262,
    BUZZER_TONE_CS4 = 277,
    BUZZER_TONE_D4  = 294,
    BUZZER_TONE_DS4 = 311,
    BUZZER_TONE_E4  = 330,
    BUZZER_TONE_F4  = 349,
    BUZZER_TONE_FS4 = 370,
    BUZZER_TONE_G4  = 392,
    BUZZER_TONE_GS4 = 415,
    BUZZER_TONE_A4  = 440,
    BUZZER_TONE_AS4 = 466,
    BUZZER_TONE_B4  = 494,
    BUZZER_TONE_C5  = 523,
    BUZZER_TONE_CS5 = 554,
    BUZZER_TONE_D5  = 587,
    BUZZER_TONE_DS5 = 622,
    BUZZER_TONE_E5  = 659,
    BUZZER_TONE_F5  = 698,
    BUZZER_TONE_FS5 = 740,
    BUZZER_TONE_G5  = 784,
    BUZZER_TONE_GS5 = 831,
    BUZZER_TONE_A5  = 880,
    BUZZER_TONE_AS5 = 932,
    BUZZER_TONE_B5  = 988,
    BUZZER_TONE_C6  = 1047,
} BuzzerTone;

/* TIM3_CH4 → PB1，供蜂鸣器使用。调用前先完成 BSP_GPIO_Init()。
 * TIM3 计数频率为 1 MHz，输出默认关闭（PB1 保持低电平）。 */
void TIM_Init(void);
/* ar 是 16 位 ARR 寄存器值，方波频率约为 1000000/(ar+1) Hz。
 * 传入值限制到 1..65535；修改时保持当前的输出开/关状态。 */
void TIM_SetAutoReload(uint32_t ar);
/* 输出约 50% 占空比的方波。 */
void TIM_EnableOutput(void);
/* 1..5 maps to a 10..50 percent PWM duty cycle. */
void TIM_SetVolume(uint8_t level);
/* 将 PWM 比较值清零，让 PB1 持续输出低电平。 */
void TIM_DisableOutput(void);

#endif //CH32CLOCK_TIM_H
