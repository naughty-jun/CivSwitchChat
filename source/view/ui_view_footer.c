#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

#define FOOTER_SEPARATOR_HEIGHT  1
#define FOOTER_TEXT_Y            683
#define FOOTER_FONT_SIZE         14

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

typedef struct {
    s32 x;
    const char* label;
    s32 max_width;
} FooterHint;

static const FooterHint BROWSE_HINTS[] = {
    {   42, "[L] Lobby",       120 },
    {  190, "[R] Logs",        120 },
    {  340, "[Y] Scan lobby",  150 },
    {  510, "[D-Pad Up/Down] Scroll",  180 },
    { 1110, "[+] Exit",        130 },
};

static const FooterHint SESSION_HINTS[] = {
    {   42, "[B] Leave",       120 },
    {  190, "[X] Send Hello",   150 },
    {  360, "[Y] Send from file",   150 },
    // { 1110, "[+] Exit",        130 },
};

static const FooterHint CHAT_BROWSE_HINTS[] = {
    {   42, "[A] Select",      120 },
    {  190, "[B] Cancel",      120 },
    {  340, "[D-Pad Up/Down] Scroll",  180 },
};

static const FooterHint CHAT_WARNING_HINTS[] = {
    {   42, "[A] Send Anyway", 180 },
    {  240, "[B] Cancel",      120 },
};

static const FooterHint CHAT_SENDING_HINTS[] = {
    {   42, "[B] Stop",        120 },
};

static void drawHints(UiDrawContext* draw, const FooterHint* hints, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        uiDrawText(
            draw, hints[i].x, FOOTER_TEXT_Y,
            hints[i].label,
            FOOTER_FONT_SIZE,
            UI_COLOR_TEXT_SECONDARY,
            hints[i].max_width
        );
    }
}

static void drawSessionHints(UiDrawContext* draw, const AppController* controller)
{
    switch (controller->chat_panel) {
        case APP_CHAT_PANEL_BROWSE:
            drawHints(draw, CHAT_BROWSE_HINTS, ARRAY_LEN(CHAT_BROWSE_HINTS));
            return;
        case APP_CHAT_PANEL_WARNING:
            drawHints(draw, CHAT_WARNING_HINTS, ARRAY_LEN(CHAT_WARNING_HINTS));
            return;
        case APP_CHAT_PANEL_SENDING:
            drawHints(draw, CHAT_SENDING_HINTS, ARRAY_LEN(CHAT_SENDING_HINTS));
            return;
        case APP_CHAT_PANEL_NONE:
            drawHints(draw, SESSION_HINTS, ARRAY_LEN(SESSION_HINTS));
            return;
    }
}

void uiViewDrawFooter(UiDrawContext* draw, const AppController* controller)
{
    uiDrawFillRect(
        draw,
        0, UI_FOOTER_Y,
        UI_SCREEN_WIDTH,
        UI_SCREEN_HEIGHT - UI_FOOTER_Y,
        UI_COLOR_FOOTER
    );

    uiDrawFillRect(
        draw,
        0, UI_FOOTER_Y,
        UI_SCREEN_WIDTH,
        FOOTER_SEPARATOR_HEIGHT,
        UI_COLOR_SEPARATOR
    );

    if (controller->screen == APP_SCREEN_SESSION)
        drawSessionHints(draw, controller);
    else
        drawHints(draw, BROWSE_HINTS, ARRAY_LEN(BROWSE_HINTS));
}
