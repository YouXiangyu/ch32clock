#include "display.h"
#include "Component/oled/oled.h"
#include <string.h>

/* Coordinates are native OLED pixels, with eight vertical bits per byte. */
static void pixel(unsigned x, unsigned y)
{
    if (x < OLED_WIDTH && y < OLED_PAGES * 8)
        g_oled.backBuffer->oledData[x + (y / 8) * OLED_WIDTH] |= (uint8_t)(1u << (y % 8));
}

static void rect(int x, int y, int width, int height)
{
    for (int row = y; row < y + height; ++row)
        for (int col = x; col < x + width; ++col) pixel(col, row);
}

static void line(unsigned page, const char *text)
{
    oled_setString(&g_oled, page, text, (uint8_t)strlen(text));
}

/* Seven-segment digits, scalable for menu values and the countdown. */
static void digit(int x, int y, unsigned n, int scale)
{
    static const uint8_t mask[] = {0x3f,6,0x5b,0x4f,0x66,0x6d,0x7d,7,0x7f,0x6f};
    static const uint8_t bars[][4] = {
        {1,0,4,1},{5,1,1,4},{5,6,1,4},{1,10,4,1},
        {0,6,1,4},{0,1,1,4},{1,5,4,1}
    };
    for (unsigned i = 0; i < 7; ++i) {
        if (mask[n] & (1u << i)) {
            const uint8_t *b = bars[i];
            rect(x+b[0]*scale, y+b[1]*scale, b[2]*scale, b[3]*scale);
        }
    }
}

static void pair(int x, int y, unsigned n, int scale)
{
    digit(x, y, n / 10, scale);
    digit(x + 7*scale, y, n % 10, scale);
}

static void durationRow(unsigned page, const char *name, unsigned minutes, bool selected)
{
    char row[] = "            00 min";
    row[0] = selected ? '>' : ' ';
    memcpy(row + 2, name, strlen(name));
    row[12] = '0' + minutes / 10;
    row[13] = '0' + minutes % 10;
    line(page, row);
}

static const char *const settingNames[] = {"Brightness", "Volume", "Mute", "Preset count"};

static void drawSettings(const Pomodoro *a, bool editingCount)
{
    line(0, "Settings");
    for (unsigned i = 0; i < SETTING_COUNT; ++i) {
        char row[] = "                     ";
        row[0] = i == a->setting ? '>' : ' ';
        memcpy(row + 2, settingNames[i], strlen(settingNames[i]));
        if (i == PRESET_COUNT && editingCount) {
            row[16] = '[';
            row[17] = '0' + a->draft;
            row[18] = ']';
        } else if (i == MUTE) strcpy(row + 17, a->prefs.value[i] ? "On" : "Off");
        else row[17] = '0' + a->prefs.value[i];
        line(2 + i, row);
    }
    const char *hint = "Tap open / Hold back";
    if (a->setting == PRESET_COUNT) hint = "Tap edit / Hold back";
    if (editingCount) hint = "Tap OK / Hold undo";
    line(7, hint);
}

void drawPomodoro(const Pomodoro *a, bool notice)
{
    oled_clearBuffer(&g_oled);
    switch (a->page) {
    case HOME:
    case PREVIEW:
    case EDIT_PRESET:
        if (a->page == HOME && a->card == a->prefs.value[PRESET_COUNT]) {
            line(0, "Settings");
            line(7, "Tap to open");
        } else {
            char title[] = "Edit Preset 1";
            title[12] = '1' + a->card;
            line(0, a->page == EDIT_PRESET ? title : title + 5);
            Preset p = a->prefs.presets[a->card];
            durationRow(2, "Focus", p.focus, a->page == EDIT_PRESET && !a->editRest);
            durationRow(4, "Break", p.rest, a->page == EDIT_PRESET && a->editRest);
            const char *hint = "Tap open / Hold edit";
            if (a->page == EDIT_PRESET) hint = "Tap next / Hold done";
            if (a->page == PREVIEW) hint = notice ? "Break complete" : "Tap start / Hold back";
            line(7, hint);
        }
        if (a->page == HOME) {
            for (unsigned i = 0; i <= a->prefs.value[PRESET_COUNT]; ++i)
                rect(99 + 7*i, 4, 4, i == a->card ? 5 : 1);
        }
        break;
    case TIMER: {
        unsigned seconds = (a->remaining + 199) / 200;
        const char *heading = a->rest ? "Break" : "Focus";
        if (a->paused) heading = a->rest ? "Break / Paused" : "Focus / Paused";
        else if (notice && a->rest) heading = "Break / Focus done";
        line(0, heading);
        pair(19, 15, seconds / 60, 3);
        rect(62, 24, 3, 3);
        rect(62, 36, 3, 3);
        pair(71, 15, seconds % 60, 3);
        line(7, a->paused ? "Tap play / Hold exit" : "Tap pause / Hold exit");
        break;
    }
    case SETTINGS:
        drawSettings(a, false);
        break;
    case EDIT_SETTING:
        if (a->setting == PRESET_COUNT) {
            drawSettings(a, true);
            break;
        }
        line(0, settingNames[a->setting]);
        if (a->setting == MUTE) line(3, a->draft ? "         On" : "         Off");
        else digit(55, 17, a->draft, 2);
        line(7, "Tap OK / Hold back");
        break;
    }
    oled_Flush(&g_oled);
}
