#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

#define COUNT_FONT_SIZE  14
#define COUNT_MAX_WIDTH  200

void uiViewDrawSectionHeading(
    UiDrawContext* draw,
    s32 title_x,
    s32 title_y,
    const char* title,
    const char* count_text,
    s32 count_x,
    s32 count_y
)
{
    uiDrawText(
        draw, title_x, title_y,
        title,
        UI_SECTION_TITLE_FONT_SIZE,
        UI_COLOR_TEXT,
        UI_SECTION_TITLE_MAX_WIDTH
    );

    if (!count_text)
        return;

    uiDrawText(
        draw, count_x, count_y,
        count_text,
        COUNT_FONT_SIZE,
        UI_COLOR_TEXT_SECONDARY,
        COUNT_MAX_WIDTH
    );
}
