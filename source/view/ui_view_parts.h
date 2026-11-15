#pragma once

/* Screen parts composed by uiViewDraw. Internal to the view layer. */

#include "ui_draw.h"
#include "../model/ldn_model.h"
#include "../controller/app_controller.h"

void uiViewDrawHeader(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
);

void uiViewDrawFooter(UiDrawContext* draw, const AppController* controller);

/*
 * Section title shown at the top of a content area, at (title_x, title_y).
 * `count_text` is drawn to its right at (count_x, count_y); pass NULL to
 * omit it (used by the session screen's two side-by-side columns).
 */
void uiViewDrawSectionHeading(
    UiDrawContext* draw,
    s32 title_x,
    s32 title_y,
    const char* title,
    const char* count_text,
    s32 count_x,
    s32 count_y
);

void uiViewDrawSession(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
);

/*
 * Draws the chat file browse/warning/sending overlay on top of the session
 * screen. Safe to call unconditionally: does nothing when chat_panel is
 * APP_CHAT_PANEL_NONE.
 */
void uiViewDrawChatPanel(
    UiDrawContext* draw,
    const AppController* controller
);

/* One colored log line: a bar at `x` and the text 18px to its right. */
void uiViewDrawLogLine(
    UiDrawContext* draw,
    const char* line,
    s32 x,
    s32 y,
    s32 max_width
);

void uiViewDrawLobby(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
);

void uiViewDrawNetworkCard(
    UiDrawContext* draw,
    const LdnNetworkInfo* info,
    s32 index,
    s32 y,
    bool selected
);

void uiViewDrawLogs(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
);
