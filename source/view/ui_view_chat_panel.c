#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

#include <stdint.h>
#include <stdio.h>

/* Overlay frame, centered on the session screen. */
#define PANEL_X             140
#define PANEL_Y             180
#define PANEL_WIDTH         1000
#define PANEL_HEIGHT        460
#define PANEL_RADIUS        16
#define PANEL_BORDER        2

#define PANEL_TITLE_X       (PANEL_X + 32)
#define PANEL_TITLE_Y       (PANEL_Y + 24)
#define PANEL_TITLE_FONT_SIZE 22
#define PANEL_TITLE_MAX_WIDTH (PANEL_WIDTH - 64)

#define PANEL_BODY_X        (PANEL_X + 32)
#define PANEL_BODY_Y        (PANEL_Y + 72)
#define PANEL_BODY_MAX_WIDTH (PANEL_WIDTH - 64)

/* File browser rows */
#define FILE_ROW_STRIDE     28
#define FILE_ROW_FONT_SIZE  16
#define FILE_ROW_INDICATOR_WIDTH 20
#define FILE_LIST_Y_OFFSET  34   /* Pushes the list down to make room for the interval line. */

/* Send interval line, shown above the file list */
#define SEND_INTERVAL_FONT_SIZE 20

/* Transient notice at the bottom of the browse/sending panels */
#define PANEL_NOTICE_Y          (PANEL_Y + PANEL_HEIGHT - 48)
#define PANEL_NOTICE_FONT_SIZE  16

/* ---- Shared frame ---- */

static void drawTransientNotice(UiDrawContext* draw, const AppController* controller)
{
    if (controller->chat_file_notice[0] == '\0' ||
        ldnModelNowMs() >= controller->chat_file_notice_until_ms)
        return;

    uiDrawText(
        draw, PANEL_BODY_X, PANEL_NOTICE_Y,
        controller->chat_file_notice,
        PANEL_NOTICE_FONT_SIZE,
        UI_COLOR_WARNING,
        PANEL_BODY_MAX_WIDTH
    );
}

static void drawPanelFrame(UiDrawContext* draw, const char* title)
{
    uiDrawRoundedBorder(
        draw,
        PANEL_X, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT,
        PANEL_RADIUS, PANEL_BORDER,
        UI_COLOR_ACCENT,
        UI_COLOR_PANEL
    );

    uiDrawText(
        draw, PANEL_TITLE_X, PANEL_TITLE_Y,
        title,
        PANEL_TITLE_FONT_SIZE,
        UI_COLOR_TEXT,
        PANEL_TITLE_MAX_WIDTH
    );
}

/* ---- Browse ---- */

static void drawSendIntervalLine(UiDrawContext* draw, const AppController* controller)
{
    char line[64];

    snprintf(
        line, sizeof(line),
        "Send delay: %d ms  (D-Pad Left/Right to adjust, %d-%d)",
        (int)controller->chat_send_interval_ms,
        APP_CHAT_SEND_INTERVAL_MIN_MS,
        APP_CHAT_SEND_INTERVAL_MAX_MS
    );

    uiDrawText(
        draw, PANEL_BODY_X, PANEL_BODY_Y,
        line,
        SEND_INTERVAL_FONT_SIZE,
        UI_COLOR_TEXT_SECONDARY,
        PANEL_BODY_MAX_WIDTH
    );
}

static void drawBrowsePanel(UiDrawContext* draw, const AppController* controller)
{
    drawPanelFrame(draw, "Select a text file (.txt)");
    drawSendIntervalLine(draw, controller);

    const ChatFileList* list = &controller->chat_file_list;
    s32 end = controller->chat_file_scroll + APP_CHAT_FILE_VISIBLE_ROWS;

    if (end > list->count)
        end = list->count;

    s32 y = PANEL_BODY_Y + FILE_LIST_Y_OFFSET;

    for (s32 i = controller->chat_file_scroll; i < end; i++) {
        bool selected = (i == controller->chat_file_selection);

        if (selected) {
            uiDrawText(
                draw, PANEL_BODY_X, y,
                ">",
                FILE_ROW_FONT_SIZE,
                UI_COLOR_SELECTION,
                FILE_ROW_INDICATOR_WIDTH
            );
        }

        uiDrawText(
            draw, PANEL_BODY_X + FILE_ROW_INDICATOR_WIDTH, y,
            list->entries[i].name,
            FILE_ROW_FONT_SIZE,
            selected ? UI_COLOR_SELECTION : UI_COLOR_TEXT,
            PANEL_BODY_MAX_WIDTH - FILE_ROW_INDICATOR_WIDTH
        );

        y += FILE_ROW_STRIDE;
    }

    drawTransientNotice(draw, controller);
}

/* ---- Warning ---- */

#define WARNING_LINE_STRIDE    30
#define WARNING_LINE_FONT_SIZE 17

static void drawWarningPanel(UiDrawContext* draw, const AppController* controller)
{
    drawPanelFrame(draw, "Large text file");

    const ChatFileContent* content = &controller->chat_file_content;
    char line[CHAT_FILE_NAME_MAX + 32];
    s32 y = PANEL_BODY_Y;

    snprintf(line, sizeof(line), "File: %s", controller->chat_file_name);
    uiDrawText(draw, PANEL_BODY_X, y, line, WARNING_LINE_FONT_SIZE,
               UI_COLOR_TEXT, PANEL_BODY_MAX_WIDTH);
    y += WARNING_LINE_STRIDE;

    snprintf(line, sizeof(line), "Lines to send: %d", (int)content->line_count);
    uiDrawText(draw, PANEL_BODY_X, y, line, WARNING_LINE_FONT_SIZE,
               chatFileNeedsLineCountWarning(content->line_count) ? UI_COLOR_WARNING : UI_COLOR_TEXT,
               PANEL_BODY_MAX_WIDTH);
    y += WARNING_LINE_STRIDE;

    snprintf(line, sizeof(line), "File size: %zu bytes", content->file_size);
    uiDrawText(draw, PANEL_BODY_X, y, line, WARNING_LINE_FONT_SIZE,
               chatFileNeedsSizeWarning(content->file_size) ? UI_COLOR_WARNING : UI_COLOR_TEXT,
               PANEL_BODY_MAX_WIDTH);
    y += WARNING_LINE_STRIDE * 2;

    if (content->line_count_exceeds_limit) {
        snprintf(line, sizeof(line), "Only the first %d lines will be sent.", CHAT_FILE_LINE_COUNT_MAX);
        uiDrawText(draw, PANEL_BODY_X, y, line, WARNING_LINE_FONT_SIZE,
                   UI_COLOR_WARNING, PANEL_BODY_MAX_WIDTH);
        y += WARNING_LINE_STRIDE;
    }

    uiDrawText(
        draw, PANEL_BODY_X, y,
        "Sending this many lines takes a while and fills the chat log.",
        WARNING_LINE_FONT_SIZE,
        UI_COLOR_TEXT_SECONDARY,
        PANEL_BODY_MAX_WIDTH
    );
    y += WARNING_LINE_STRIDE;

    uiDrawText(
        draw, PANEL_BODY_X, y,
        "Press A to send anyway, or B to cancel.",
        WARNING_LINE_FONT_SIZE,
        UI_COLOR_TEXT,
        PANEL_BODY_MAX_WIDTH
    );
}

/* ---- Sending ---- */

#define SENDING_LINE_FONT_SIZE   18
#define SENDING_PROGRESS_Y_OFFSET 50
#define SENDING_BAR_Y_OFFSET     90
#define SENDING_BAR_WIDTH        (PANEL_WIDTH - 64)
#define SENDING_BAR_HEIGHT       18
#define SENDING_BAR_RADIUS       6
#define SENDING_CURRENT_Y_OFFSET 130

static void drawSendingPanel(UiDrawContext* draw, const AppController* controller)
{
    drawPanelFrame(draw, "Sending chat from text file");

    const ChatFileContent* content = &controller->chat_file_content;
    s32 sent = controller->chat_send_index;
    s32 total = content->line_count;
    char line[CHAT_FILE_LINE_MAX + 32];

    snprintf(line, sizeof(line), "File: %s", controller->chat_file_name);
    uiDrawText(draw, PANEL_BODY_X, PANEL_BODY_Y, line, SENDING_LINE_FONT_SIZE,
               UI_COLOR_TEXT, PANEL_BODY_MAX_WIDTH);

    snprintf(line, sizeof(line), "Progress: %d / %d", (int)sent, (int)total);
    uiDrawText(
        draw, PANEL_BODY_X, PANEL_BODY_Y + SENDING_PROGRESS_Y_OFFSET,
        line, SENDING_LINE_FONT_SIZE,
        UI_COLOR_TEXT_SECONDARY, PANEL_BODY_MAX_WIDTH
    );

    uiDrawRoundedRect(
        draw,
        PANEL_BODY_X, PANEL_BODY_Y + SENDING_BAR_Y_OFFSET,
        SENDING_BAR_WIDTH, SENDING_BAR_HEIGHT,
        SENDING_BAR_RADIUS,
        UI_COLOR_PANEL_BORDER
    );

    s32 filled_width = total > 0 ? (s32)((int64_t)SENDING_BAR_WIDTH * sent / total) : 0;
    if (filled_width > 0) {
        uiDrawRoundedRect(
            draw,
            PANEL_BODY_X, PANEL_BODY_Y + SENDING_BAR_Y_OFFSET,
            filled_width, SENDING_BAR_HEIGHT,
            SENDING_BAR_RADIUS,
            UI_COLOR_ACCENT
        );
    }

    if (sent < total) {
        snprintf(line, sizeof(line), "Next: %s", content->lines[sent]);
        uiDrawText(
            draw, PANEL_BODY_X, PANEL_BODY_Y + SENDING_CURRENT_Y_OFFSET,
            line, SENDING_LINE_FONT_SIZE,
            UI_COLOR_TEXT, PANEL_BODY_MAX_WIDTH
        );
    }

    drawTransientNotice(draw, controller);
}

/* ---- Entry point ---- */

void uiViewDrawChatPanel(
    UiDrawContext* draw,
    const AppController* controller
)
{
    switch (controller->chat_panel) {
        case APP_CHAT_PANEL_BROWSE:
            drawBrowsePanel(draw, controller);
            return;
        case APP_CHAT_PANEL_WARNING:
            drawWarningPanel(draw, controller);
            return;
        case APP_CHAT_PANEL_SENDING:
            drawSendingPanel(draw, controller);
            return;
        case APP_CHAT_PANEL_NONE:
            return;
    }
}
