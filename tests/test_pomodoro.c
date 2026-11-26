#include "App/pomodoro.h"
#include <assert.h>
#include <stdio.h>

static void testPresetCount(void)
{
    Pomodoro app = {.prefs = defaultPreferences};
    turn(&app, -1); /* settings card */
    press(&app, CLICK);
    turn(&app, -1);
    for (int count = 1; count <= 3; ++count) {
        assert(app.setting == PRESET_COUNT);
        press(&app, CLICK);
        turn(&app, count - app.draft);
        press(&app, CLICK);
        press(&app, BACK);
        assert(app.card == count && app.prefs.value[PRESET_COUNT] == count);
        turn(&app, 1);
        assert(app.card == 0);
        turn(&app, count);
        press(&app, CLICK);
        turn(&app, -1);
    }
    assert(app.prefs.presets[1].focus == 50 && app.prefs.presets[2].focus == 15);
}

static void testTimer(void)
{
    Pomodoro app = {.prefs = defaultPreferences};
    press(&app, BACK); /* edit first preset */
    turn(&app, 1);
    press(&app, CLICK);
    turn(&app, -1);
    press(&app, BACK);
    assert(app.prefs.presets[0].focus == 26 && app.prefs.presets[0].rest == 4);

    press(&app, CLICK);
    press(&app, CLICK);
    assert(app.remaining == 312000);
    advanceTimer(&app, 1);
    press(&app, CLICK);
    advanceTimer(&app, 5000);
    assert(app.remaining == 311999);
    press(&app, CLICK);
    assert(advanceTimer(&app, 312001) && app.rest && app.remaining == 47998);
    assert(advanceTimer(&app, 47998) && app.page == PREVIEW);
    press(&app, CLICK);
    press(&app, BACK);
    assert(app.page == PREVIEW);
}

static void testSettings(void)
{
    Pomodoro app = {.prefs = defaultPreferences};
    turn(&app, -1);
    press(&app, CLICK);
    for (int setting = BRIGHTNESS; setting <= MUTE; ++setting) {
        uint8_t original = app.prefs.value[setting];
        press(&app, CLICK);
        turn(&app, 1);
        press(&app, BACK);
        assert(app.prefs.value[setting] == original);

        press(&app, CLICK);
        turn(&app, 1);
        press(&app, CLICK);
        assert(app.prefs.value[setting] == original + 1);
        turn(&app, 1);
    }
}

static void startTap(Button *button, uint32_t tick)
{
    assert(sampleButton(button, true, tick) == NO_CLICK);
    assert(sampleButton(button, true, tick + 4) == NO_CLICK);
    assert(sampleButton(button, false, tick + 10) == NO_CLICK);
}

static void testGestures(void)
{
    Button button = {0};
    startTap(&button, 10);
    assert(sampleButton(&button, false, 24) == NO_CLICK);
    assert(sampleButton(&button, false, 84) == CLICK);

    startTap(&button, 100);
    assert(sampleButton(&button, false, 114) == NO_CLICK);
    startTap(&button, 130);
    assert(sampleButton(&button, false, 144) == BACK);
    assert(sampleButton(&button, false, 220) == NO_CLICK);

    sampleButton(&button, true, 0xfffffff0u);
    sampleButton(&button, true, 0xfffffff4u);
    assert(sampleButton(&button, true, 128) == BACK);
    sampleButton(&button, false, 132);
    sampleButton(&button, false, 136);
    assert(sampleButton(&button, false, 222) == NO_CLICK);
}

int main(void)
{
    testPresetCount();
    testTimer();
    testSettings();
    testGestures();
    puts("Passed preset navigation, timer transitions, settings and gesture tests.");
}
