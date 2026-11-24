#include <stdint.h>
#include "systemInit.h"
#include "clocktick.h"
#include "Component/oled/oled.h"
#include "bootLoader/bootCfg.h"

/* 固定宽度，数字变长或毫秒计数回绕时不会留下上一帧的字符。 */
static void format_milliseconds(char text[15], uint32_t milliseconds)
{
    text[0] = 'm';
    text[1] = 's';
    text[2] = ':';
    text[3] = ' ';
    text[14] = '\0';

    for (uint8_t i = 14U; i > 4U;) {
        --i;
        text[i] = (char)('0' + milliseconds % 10U);
        milliseconds /= 10U;
    }
}

int main(void)
{
    BSP_SystemInit();
    if (oled_Init(&g_oled) != OLED_OK) {
        for (;;) { }
    }

    if (goBootloader != 0u) {
        enterBootloader();
        oled_clearBuffer(&g_oled); /* EC11 取消后，清掉启动模式文字再绘制正常页面。 */
    }

    char timeText[15];
    uint32_t lastTick = clockTick5ms;
    format_milliseconds(timeText, lastTick * 5U);
    oled_setString(&g_oled, 0U, "Hello World", 11U);
    oled_setString(&g_oled, 2U, timeText, 14U);
    oled_Flush(&g_oled);

    for (;;) {
        OLED_Service(&g_oled);

        /* 5 ms 更新一次后台数字；DMA 按总线速度发送最新的完整帧。 */
        uint32_t tick = clockTick5ms;
        if (tick != lastTick) {
            lastTick = tick;
            format_milliseconds(timeText, tick * 5U);
            oled_setString(&g_oled, 2U, timeText, 14U);
            oled_Flush(&g_oled);
        }
    }
}
