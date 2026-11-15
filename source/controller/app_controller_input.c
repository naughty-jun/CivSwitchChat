#include "app_controller_input.h"

#define DPAD_DIRECTIONS \
    (HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right)

/* Of the directions currently held, picks the one to track (0 if none). */
static u64 pickTrackedDirection(u64 held_directions)
{
    if (held_directions & HidNpadButton_Up)
        return HidNpadButton_Up;
    if (held_directions & HidNpadButton_Down)
        return HidNpadButton_Down;
    if (held_directions & HidNpadButton_Left)
        return HidNpadButton_Left;
    if (held_directions & HidNpadButton_Right)
        return HidNpadButton_Right;
    return 0;
}

u64 appControllerApplyDpadRepeat(AppController* controller, PadState* pad, u64 buttons_down)
{
    u64 held_directions = padGetButtons(pad) & DPAD_DIRECTIONS;
    u64 tracked = pickTrackedDirection(held_directions);

    if (tracked != controller->dpad_held_button) {
        controller->dpad_held_button = tracked;
        controller->dpad_next_repeat_ms = ldnModelNowMs() + APP_DPAD_REPEAT_DELAY_MS;
        return buttons_down;
    }

    if (!tracked)
        return buttons_down;

    u64 now = ldnModelNowMs();
    if (now < controller->dpad_next_repeat_ms)
        return buttons_down;

    controller->dpad_next_repeat_ms = now + APP_DPAD_REPEAT_INTERVAL_MS;

    return buttons_down | tracked;
}

/* Clamps `*selection` into [0, count - 1] (count is assumed > 0). */
static void clampSelection(s32* selection, s32 count)
{
    if (*selection >= count)
        *selection = count - 1;

    if (*selection < 0)
        *selection = 0;
}

void appControllerMoveSelection(
    s32* selection,
    s32* scroll,
    u64 buttons,
    s32 count,
    s32 visible_count
)
{
    clampSelection(selection, count);

    if ((buttons & HidNpadButton_Up) && *selection > 0)
        (*selection)--;

    if ((buttons & HidNpadButton_Down) && *selection < count - 1)
        (*selection)++;

    if (*selection < *scroll)
        *scroll = *selection;

    if (*selection >= *scroll + visible_count)
        *scroll = *selection - visible_count + 1;
}
