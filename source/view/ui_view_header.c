#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

/* Title */
#define TITLE_X               42
#define TITLE_Y               25
#define TITLE_FONT_SIZE       30
#define TITLE_MAX_WIDTH       500

#define SUBTITLE_X            44
#define SUBTITLE_Y            69
#define SUBTITLE_FONT_SIZE    15
#define SUBTITLE_MAX_WIDTH    500

/* Station status badge */
#define BADGE_X               930
#define BADGE_Y               34
#define BADGE_WIDTH           300
#define BADGE_HEIGHT          42
#define BADGE_RADIUS          12
#define BADGE_BORDER          1

#define BADGE_DOT_X           950
#define BADGE_DOT_Y           49
#define BADGE_DOT_SIZE        10
#define BADGE_DOT_RADIUS      5

#define BADGE_TEXT_X          972
#define BADGE_TEXT_Y          43
#define BADGE_TEXT_FONT_SIZE  16
#define BADGE_TEXT_MAX_WIDTH  240

/* Tabs */
#define TAB_Y                 108
#define TAB_WIDTH             590
#define TAB_HEIGHT            26
#define TAB_RADIUS            8
#define TAB_BORDER            1
#define LOBBY_TAB_X           40
#define LOGS_TAB_X            650

#define TAB_LABEL_Y           111
#define TAB_LABEL_FONT_SIZE   14
#define TAB_LABEL_MAX_WIDTH   150 // 100
#define LOBBY_TAB_LABEL_X     310
#define LOGS_TAB_LABEL_X      920

/* Single full-width tab shown on the session screen */
#define SESSION_TAB_X         40
#define SESSION_TAB_WIDTH     1200
#define SESSION_TAB_LABEL_X   585

#define SEPARATOR_HEIGHT      1

static void drawTitle(UiDrawContext* draw)
{
    uiDrawText(
        draw, TITLE_X, TITLE_Y,
        "CivSwitchChat",
        TITLE_FONT_SIZE,
        UI_COLOR_TEXT,
        TITLE_MAX_WIDTH
    );

    char *subtitle_text;
    u32 subtitle_color;
    if (appletGetAppletType() == AppletType_LibraryApplet) {
        subtitle_text = "You can send messages in your Civ VII lobby ;)";
        subtitle_color = UI_COLOR_TEXT_SECONDARY;
    } else {
        subtitle_text = "This app supports only applet mode :(";
        subtitle_color = UI_COLOR_WARNING;
    }
    uiDrawText(
        draw, SUBTITLE_X, SUBTITLE_Y,
        subtitle_text,
        SUBTITLE_FONT_SIZE,
        subtitle_color,
        SUBTITLE_MAX_WIDTH
    );
}

static void drawStationBadge(UiDrawContext* draw, const LdnModel* model)
{
    u32 status_color = model->station_open
        ? UI_COLOR_SUCCESS
        : UI_COLOR_WARNING;

    const char* status_text = model->station_open
        ? "STATION READY"
        : "STATION UNAVAILABLE";

    uiDrawRoundedBorder(
        draw,
        BADGE_X, BADGE_Y, BADGE_WIDTH, BADGE_HEIGHT,
        BADGE_RADIUS, BADGE_BORDER,
        UI_COLOR_PANEL_BORDER,
        UI_COLOR_PANEL
    );

    uiDrawRoundedRect(
        draw,
        BADGE_DOT_X, BADGE_DOT_Y, BADGE_DOT_SIZE, BADGE_DOT_SIZE,
        BADGE_DOT_RADIUS,
        status_color
    );

    uiDrawText(
        draw, BADGE_TEXT_X, BADGE_TEXT_Y,
        status_text,
        BADGE_TEXT_FONT_SIZE,
        status_color,
        BADGE_TEXT_MAX_WIDTH
    );
}

static void drawTab(
    UiDrawContext* draw,
    s32 x,
    s32 width,
    s32 label_x,
    const char* label,
    bool active
)
{
    uiDrawRoundedBorder(
        draw,
        x, TAB_Y, width, TAB_HEIGHT,
        TAB_RADIUS, TAB_BORDER,
        active ? UI_COLOR_ACCENT : UI_COLOR_PANEL_BORDER,
        active ? UI_COLOR_ACCENT_DARK : UI_COLOR_PANEL
    );

    uiDrawText(
        draw, label_x, TAB_LABEL_Y,
        label,
        TAB_LABEL_FONT_SIZE,
        UI_COLOR_TEXT,
        TAB_LABEL_MAX_WIDTH
    );
}

static void drawTabs(UiDrawContext* draw, const AppController* controller)
{
    if (controller->screen == APP_SCREEN_SESSION) {
        drawTab(draw, SESSION_TAB_X, SESSION_TAB_WIDTH, SESSION_TAB_LABEL_X,
                "LOBBY SESSION", true);
        return;
    }

    drawTab(draw, LOBBY_TAB_X, TAB_WIDTH, LOBBY_TAB_LABEL_X, "LOBBY",
            controller->tab == APP_TAB_LOBBY);
    drawTab(draw, LOGS_TAB_X, TAB_WIDTH, LOGS_TAB_LABEL_X, "LOGS",
            controller->tab == APP_TAB_LOG);
}

void uiViewDrawHeader(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
)
{
    uiDrawFillRect(
        draw,
        0, 0,
        UI_SCREEN_WIDTH,
        UI_HEADER_HEIGHT,
        UI_COLOR_HEADER
    );

    drawTitle(draw);
    drawStationBadge(draw, model);

    drawTabs(draw, controller);

    uiDrawFillRect(
        draw,
        0, UI_HEADER_HEIGHT - SEPARATOR_HEIGHT,
        UI_SCREEN_WIDTH,
        SEPARATOR_HEIGHT,
        UI_COLOR_SEPARATOR
    );
}
