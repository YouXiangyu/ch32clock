#ifndef POMODORO_H
#define POMODORO_H
#include <stdbool.h>
#include <stdint.h>

typedef enum { HOME, PREVIEW, EDIT_PRESET, TIMER, SETTINGS, EDIT_SETTING } Page;
typedef enum { BRIGHTNESS, VOLUME, MUTE, PRESET_COUNT, SETTING_COUNT } Setting;
typedef enum { NO_CLICK, CLICK, BACK } Gesture;

typedef struct {
    uint8_t focus, rest;
} Preset;

typedef struct {
    Preset presets[3];
    uint8_t value[SETTING_COUNT];
} Preferences;

typedef struct {
    Preferences prefs;
    Page page;
    uint8_t card, setting, draft;
    bool rest, paused, editRest;
    uint32_t remaining;  /* 5 ms ticks */
} Pomodoro;

extern const Preferences defaultPreferences;
void turn(Pomodoro *app, int steps);
void press(Pomodoro *app, Gesture gesture);
/* Returns true when a phase ends and the buzzer should ring. */
bool advanceTimer(Pomodoro *app, uint32_t elapsed);

typedef struct {
    bool raw, stable, pending, consumed;
    uint32_t changedAt, pressedAt, releasedAt;
} Button;
Gesture sampleButton(Button *button, bool down, uint32_t now);
#endif
