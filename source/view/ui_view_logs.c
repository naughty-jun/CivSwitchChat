#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

#include <stdio.h>
#include <string.h>

#define LOG_COUNT_X             1050
#define LOG_COUNT_Y             160

#define EMPTY_MESSAGE_Y         260

#define LOG_LIST_Y              198
#define LOG_LINE_STRIDE         25

#define LOG_BAR_X               44
#define LOG_BAR_Y_OFFSET        5
#define LOG_BAR_WIDTH           4
#define LOG_BAR_HEIGHT          19
#define LOG_BAR_RADIUS          2

#define LOG_TEXT_X_OFFSET       18
#define LOG_TEXT_FONT_SIZE      14
#define LOG_TEXT_MAX_WIDTH      1150

static void drawLogsHeading(UiDrawContext* draw, u32 count)
{
    char count_text[64];

    snprintf(count_text, sizeof(count_text), "ENTRIES  %u", count);

    uiViewDrawSectionHeading(
        draw, UI_SECTION_TITLE_X, UI_SECTION_TITLE_Y, "Application Logs",
        count_text, LOG_COUNT_X, LOG_COUNT_Y
    );
}

static u32 selectLineColor(const char* line)
{
    if (strstr(line, "ERROR") || strstr(line, "FAIL"))
        return UI_COLOR_ERROR;

    if (strstr(line, "WARN"))
        return UI_COLOR_WARNING;

    if (strstr(line, "SUCCESS") || strstr(line, "READY"))
        return UI_COLOR_SUCCESS;

    return UI_COLOR_TEXT_SECONDARY;
}

void uiViewDrawLogLine(
    UiDrawContext* draw,
    const char* line,
    s32 x,
    s32 y,
    s32 max_width
)
{
    u32 color = selectLineColor(line);

    uiDrawRoundedRect(
        draw,
        x, y + LOG_BAR_Y_OFFSET,
        LOG_BAR_WIDTH, LOG_BAR_HEIGHT,
        LOG_BAR_RADIUS,
        color
    );

    uiDrawText(
        draw, x + LOG_TEXT_X_OFFSET, y,
        line,
        LOG_TEXT_FONT_SIZE,
        color,
        max_width
    );
}

static void drawLogLines(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model,
    u32 count
)
{
    s32 end = (s32)count - controller->log_scroll;

    if (end < 0)
        end = 0;

    if (end > (s32)count)
        end = (s32)count;

    s32 start = end - APP_LOG_VISIBLE_LINES;

    if (start < 0)
        start = 0;

    s32 y = LOG_LIST_Y;

    for (s32 i = start; i < end; i++) {
        const char* line = ldnModelGetLogLine(model, (u32)i);

        if (!line)
            continue;

        uiViewDrawLogLine(draw, line, LOG_BAR_X, y, LOG_TEXT_MAX_WIDTH);
        y += LOG_LINE_STRIDE;
    }
}

void uiViewDrawLogs(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
)
{
    u32 count = ldnModelGetLogCount(model);

    drawLogsHeading(draw, count);

    if (count == 0) {
        uiDrawText(
            draw, UI_MESSAGE_X, EMPTY_MESSAGE_Y,
            "No log entries.",
            UI_MESSAGE_FONT_SIZE,
            UI_COLOR_TEXT_SECONDARY,
            UI_MESSAGE_MAX_WIDTH
        );
        return;
    }

    drawLogLines(draw, controller, model, count);
}
