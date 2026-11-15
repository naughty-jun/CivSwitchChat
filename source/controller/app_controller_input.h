#pragma once

/*
 * D-Pad input helpers shared by every screen that scrolls a list or adjusts
 * a value: hold-to-repeat, and moving a selection within a scrolling window.
 */

#include "app_controller.h"

/*
 * Updates the controller's D-Pad repeat state and returns the buttons to act
 * on this frame: newly pressed buttons, plus the single D-Pad direction
 * currently being held once it has repeated (first after
 * APP_DPAD_REPEAT_DELAY_MS, then every APP_DPAD_REPEAT_INTERVAL_MS). Only
 * Up/Down/Left/Right repeat; all other buttons pass through unchanged.
 */
u64 appControllerApplyDpadRepeat(AppController* controller, PadState* pad, u64 buttons_down);

/*
 * Moves `*selection` by one with Up/Down, clamped to [0, count - 1], then
 * adjusts `*scroll` so the selection stays within a window of `visible_count`
 * items. Used by the lobby list, the chat file list and similar screens.
 * `count` must be > 0; callers already skip this when their list is empty.
 */
void appControllerMoveSelection(
    s32* selection,
    s32* scroll,
    u64 buttons,
    s32 count,
    s32 visible_count
);
