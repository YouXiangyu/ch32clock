#include "pomodoro.h"

const Preferences defaultPreferences = {
    .presets = {{25, 5}, {50, 10}, {15, 3}},
    .value = {3, 2, 0, 2},
};
static const uint8_t minimum[] = {1, 1, 0, 1};
static const uint8_t maximum[] = {5, 5, 1, 3};
enum { TICKS_PER_MINUTE = 12000 };

static uint8_t clamp(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static uint8_t cycle(int index, int steps, int count)
{
    return ((index + steps) % count + count) % count;
}

void turn(Pomodoro *app, int steps)
{
    switch (app->page) {
    case HOME:
        app->card = cycle(app->card, steps, app->prefs.value[PRESET_COUNT] + 1);
        break;
    case EDIT_PRESET: {
        Preset *p = &app->prefs.presets[app->card];
        uint8_t *minutes = app->editRest ? &p->rest : &p->focus;
        *minutes = clamp(*minutes + steps, 1, 99);
        break;
    }
    case SETTINGS:
        app->setting = cycle(app->setting, steps, SETTING_COUNT);
        break;
    case EDIT_SETTING:
        app->draft = clamp(app->draft + steps, minimum[app->setting], maximum[app->setting]);
        break;
    default: break;
    }
}

void press(Pomodoro *app, Gesture gesture)
{
    if (gesture == NO_CLICK) return;
    bool back = gesture == BACK;
    switch (app->page) {
    case HOME:
        if (app->card == app->prefs.value[PRESET_COUNT]) {
            if (!back) {
                app->page = SETTINGS;
                app->setting = BRIGHTNESS;
            }
        } else {
            app->editRest = false;
            app->page = back ? EDIT_PRESET : PREVIEW;
        }
        break;
    case PREVIEW:
        if (back) app->page = HOME;
        else {
            app->page = TIMER;
            app->rest = app->paused = false;
            app->remaining = app->prefs.presets[app->card].focus * TICKS_PER_MINUTE;
        }
        break;
    case TIMER:
        if (back) app->page = PREVIEW;
        else app->paused = !app->paused;
        break;
    case EDIT_PRESET:
        if (back) {
            app->page = HOME;
            break;
        }
        app->editRest = !app->editRest;
        break;
    case SETTINGS:
        if (back) app->page = HOME;
        else {
            app->draft = app->prefs.value[app->setting];
            app->page = EDIT_SETTING;
        }
        break;
    case EDIT_SETTING:
        app->page = SETTINGS;
        if (!back) {
            app->prefs.value[app->setting] = app->draft;
            app->card = app->prefs.value[PRESET_COUNT]; /* Keep settings card selected. */
        }
        break;
    }
}

bool advanceTimer(Pomodoro *app, uint32_t elapsed)
{
    if (app->page != TIMER || app->paused) return false;
    if (elapsed < app->remaining) {
        app->remaining -= elapsed;
        return false;
    }
    elapsed -= app->remaining;
    uint32_t breakTicks = app->prefs.presets[app->card].rest * TICKS_PER_MINUTE;
    if (!app->rest && elapsed < breakTicks) {
        app->rest = true;
        app->remaining = breakTicks - elapsed;
    } else {
        app->page = PREVIEW;
        app->remaining = 0;
    }
    return true;
}

Gesture sampleButton(Button *b, bool down, uint32_t now)
{
    enum { DEBOUNCE = 4, DOUBLE = 60, HOLD = 140 }; /* 20, 300 and 700 ms */
    if (down != b->raw) {
        b->raw = down;
        b->changedAt = now;
    }
    if (b->raw != b->stable && now - b->changedAt >= DEBOUNCE) {
        b->stable = down;
        if (down) {
            b->pressedAt = now;
            b->consumed = false;
        } else if (!b->consumed) {
            if (b->pending) {
                b->pending = false;
                return BACK;
            }
            b->pending = true;
            b->releasedAt = now;
        }
    }
    if (b->stable && !b->consumed && now - b->pressedAt >= HOLD) {
        b->consumed = true;
        b->pending = false;
        return BACK;
    }
    if (b->pending && !b->stable && !b->raw && now - b->releasedAt >= DOUBLE) {
        b->pending = false;
        return CLICK;
    }
    return NO_CLICK;
}
