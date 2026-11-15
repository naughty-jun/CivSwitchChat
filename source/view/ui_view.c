#include "ui_view.h"
#include "ui_theme.h"
#include "ui_view_parts.h"

static void drawContent(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
)
{
    if (controller->screen == APP_SCREEN_SESSION) {
        uiViewDrawSession(draw, controller, model);
        uiViewDrawChatPanel(draw, controller); /* No-op when chat_panel is APP_CHAT_PANEL_NONE. */
        return;
    }

    if (controller->tab == APP_TAB_LOBBY)
        uiViewDrawLobby(draw, controller, model);
    else
        uiViewDrawLogs(draw, controller, model);
}

void uiViewDraw(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
)
{
    if (!draw || !controller || !model)
        return;

    uiDrawClear(draw, UI_COLOR_BACKGROUND);

    uiViewDrawHeader(draw, controller, model);

    drawContent(draw, controller, model);

    uiViewDrawFooter(draw, controller);
}
