#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

#include <stdio.h>

#define NETWORK_COUNT_X           1030
#define NETWORK_COUNT_Y           160

#define MESSAGE_Y                 250
#define SCAN_ERROR_MAX_WIDTH      1000

#define EMPTY_MESSAGE_Y           270
#define EMPTY_HINT_Y              310
#define EMPTY_HINT_FONT_SIZE      15

#define CARD_STRIDE               151

#define RANGE_TEXT_X              1080
#define RANGE_TEXT_Y              638
#define RANGE_TEXT_FONT_SIZE      12
#define RANGE_TEXT_MAX_WIDTH      150

static void drawLobbyHeading(UiDrawContext* draw, const LdnModel* model)
{
    char count_text[64];

    snprintf(
        count_text,
        sizeof(count_text),
        "NETWORKS  %d",
        (int)model->network_count
    );

    uiViewDrawSectionHeading(
        draw, UI_SECTION_TITLE_X, UI_SECTION_TITLE_Y, "Available Networks",
        count_text, NETWORK_COUNT_X, NETWORK_COUNT_Y
    );
}

static void drawMessage(
    UiDrawContext* draw,
    s32 y,
    const char* text,
    u32 color,
    s32 max_width
)
{
    uiDrawText(
        draw, UI_MESSAGE_X, y,
        text,
        UI_MESSAGE_FONT_SIZE,
        color,
        max_width
    );
}

static void drawNoNetworksMessage(UiDrawContext* draw)
{
    drawMessage(
        draw, EMPTY_MESSAGE_Y,
        "No networks found.",
        UI_COLOR_TEXT_SECONDARY,
        UI_MESSAGE_MAX_WIDTH
    );

    uiDrawText(
        draw, UI_MESSAGE_X, EMPTY_HINT_Y,
        "Press Y to scan again.",
        EMPTY_HINT_FONT_SIZE,
        UI_COLOR_TEXT_MUTED,
        UI_MESSAGE_MAX_WIDTH
    );
}

static void drawScanFailedMessage(UiDrawContext* draw, const LdnModel* model)
{
    char error_text[80];

    snprintf(
        error_text,
        sizeof(error_text),
        "Network scan failed: 0x%08X",
        (unsigned int)model->scan_result
    );

    drawMessage(
        draw, MESSAGE_Y,
        error_text,
        UI_COLOR_ERROR,
        SCAN_ERROR_MAX_WIDTH
    );
}

/* Draws the reason there is nothing to list. Returns false when networks can be listed. */
static bool drawUnavailableMessage(UiDrawContext* draw, const LdnModel* model)
{
    if (!model->initialized) {
        drawMessage(draw, MESSAGE_Y, "LDN service is unavailable.",
                    UI_COLOR_ERROR, UI_MESSAGE_MAX_WIDTH);
        return true;
    }

    if (!model->station_open) {
        drawMessage(draw, MESSAGE_Y, "Station is unavailable.",
                    UI_COLOR_WARNING, UI_MESSAGE_MAX_WIDTH);
        return true;
    }

    if (!model->scan_attempted) {
        drawMessage(draw, MESSAGE_Y, "Waiting for network scan...",
                    UI_COLOR_TEXT_SECONDARY, UI_MESSAGE_MAX_WIDTH);
        return true;
    }

    if (!model->scan_success) {
        drawScanFailedMessage(draw, model);
        return true;
    }

    if (model->network_count == 0) {
        drawNoNetworksMessage(draw);
        return true;
    }

    return false;
}

static void drawNetworkCards(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model,
    s32 start,
    s32 end
)
{
    s32 y = UI_CARD_LIST_Y;

    for (s32 i = start; i < end; i++) {
        uiViewDrawNetworkCard(
            draw,
            &model->networks[i],
            i,
            y,
            i == controller->lobby_selection
        );

        y += CARD_STRIDE;
    }
}

static void drawRangeText(UiDrawContext* draw, const LdnModel* model, s32 start, s32 end)
{
    char range_text[64];

    snprintf(
        range_text,
        sizeof(range_text),
        "%d-%d OF %d",
        (int)(start + 1),
        (int)end,
        (int)model->network_count
    );

    uiDrawText(
        draw, RANGE_TEXT_X, RANGE_TEXT_Y,
        range_text,
        RANGE_TEXT_FONT_SIZE,
        UI_COLOR_TEXT_MUTED,
        RANGE_TEXT_MAX_WIDTH
    );
}

void uiViewDrawLobby(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
)
{
    drawLobbyHeading(draw, model);

    if (drawUnavailableMessage(draw, model))
        return;

    s32 start = controller->lobby_scroll;

    if (start < 0)
        start = 0;

    if (start >= model->network_count)
        start = model->network_count - 1;

    s32 end = start + APP_LOBBY_VISIBLE_CARDS;

    if (end > model->network_count)
        end = model->network_count;

    drawNetworkCards(draw, controller, model, start, end);
    drawRangeText(draw, model, start, end);
}
