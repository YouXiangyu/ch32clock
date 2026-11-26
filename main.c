#include "App/display.h"
#include "Component/oled/oled.h"
#include "Component/EC11/EC11.h"
#include "systemInit.h"
#include "clocktick.h"
#include "TIM.h"
#include "ch32v20x.h"

static int takeRotation(void)
{
    NVIC_DisableIRQ(EXTI9_5_IRQn);
    int steps = (int)g_ec11.forwardCount - g_ec11.reverseCount;
    g_ec11.forwardCount = g_ec11.reverseCount = g_ec11.pressCount = 0;
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    return steps;
}

int main(void)
{
    BSP_SystemInit();
    if (oled_Init(&g_oled) != OLED_OK) for (;;) { }

    Pomodoro app = {.prefs = defaultPreferences};
    takeRotation();
    bool down = !(GPIOA->INDR & (1u << 5));
    Button button = {.raw = down, .stable = down, .consumed = down};
    bool notice = false, ringing = false;
    uint32_t last = clockTick5ms, alertAt = last;
    static const uint8_t contrast[] = {0, 16, 48, 96, 160, 255};
    oled_setContrast(contrast[app.prefs.value[BRIGHTNESS]]);
    drawPomodoro(&app, false);

    for (;;) {
        OLED_Service(&g_oled);
        uint32_t now = clockTick5ms;
        uint32_t elapsed = now - last;
        if (!elapsed) continue;
        last = now;
        uint32_t oldSeconds = (app.remaining + 199) / 200;
        bool expired = advanceTimer(&app, elapsed);
        if (expired) {
            alertAt = now;
            notice = true;
            if (!app.prefs.value[MUTE]) {
                TIM_SetVolume(app.prefs.value[VOLUME]);
                ringing = true;
            }
        }
        if (ringing && now - alertAt >= 120) {
            TIM_DisableOutput();
            ringing = false;
        }
        bool clearNotice = notice && now - alertAt >= 400;
        if (clearNotice) notice = false;

        int steps = takeRotation();
        Gesture gesture = sampleButton(&button, !(GPIOA->INDR & (1u << 5)), now);
        if (steps) turn(&app, steps);
        if (gesture != NO_CLICK) {
            notice = false;
            press(&app, gesture);
        }
        if (steps || gesture || expired || clearNotice ||
            (app.remaining + 199) / 200 != oldSeconds) {
            unsigned level = app.page == EDIT_SETTING && app.setting == BRIGHTNESS ?
                             app.draft : app.prefs.value[BRIGHTNESS];
            oled_setContrast(contrast[level]);
            drawPomodoro(&app, notice);
        }
    }
}
