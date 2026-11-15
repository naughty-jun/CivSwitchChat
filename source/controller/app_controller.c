#include "app_controller.h"
#include "app_controller_chat.h"
#include "app_controller_input.h"

#include <stdio.h>
#include <switch.h>

void appControllerInitialize(
    AppController* controller,
    LdnModel* model,
    const char* nro_directory
)
{
    controller->screen = APP_SCREEN_BROWSE;
    controller->tab = APP_TAB_LOBBY;
    controller->pending_join_index = APP_NO_PENDING_JOIN;
    controller->lobby_scroll = 0;
    controller->lobby_selection = 0;
    controller->log_scroll = 0;

    controller->chat_panel = APP_CHAT_PANEL_NONE;
    controller->chat_file_selection = 0;
    controller->chat_file_scroll = 0;
    controller->chat_send_index = 0;
    controller->chat_next_send_ms = 0;
    controller->chat_send_interval_ms = APP_CHAT_SEND_INTERVAL_DEFAULT_MS;
    controller->chat_file_notice[0] = '\0';
    controller->chat_file_notice_until_ms = 0;
    snprintf(controller->nro_directory, sizeof(controller->nro_directory), "%s", nro_directory);

    controller->dpad_held_button = 0;
    controller->dpad_next_repeat_ms = 0;

    controller->model = model;
}

static void resetScrollPositions(AppController* controller)
{
    controller->lobby_selection = 0;
    controller->lobby_scroll = 0;
    controller->log_scroll = 0;
}

static void updateTab(AppController* controller, u64 buttons)
{
    if (buttons & HidNpadButton_L)
        controller->tab = APP_TAB_LOBBY;

    if (buttons & HidNpadButton_R)
        controller->tab = APP_TAB_LOG;
}

static void refreshNetworks(AppController* controller, u64 buttons)
{
    if (!(buttons & HidNpadButton_Y))
        return;

    ldnModelRefresh(controller->model);
    resetScrollPositions(controller);
}

static s32 getClampedNetworkCount(const AppController* controller)
{
    s32 count = controller->model->network_count;

    if (count < 0)
        return 0;

    if (count > LDN_MAX_NETWORKS)
        return LDN_MAX_NETWORKS;

    return count;
}

/*
 * Switches to the session screen right away. The blocking connection runs on
 * the next update, so the screen is drawn once before the app stalls.
 */
static void beginJoin(AppController* controller)
{
    s32 index = controller->lobby_selection;

    if (!ldnModelBeginSession(controller->model, index))
        return;

    controller->pending_join_index = index;
    controller->screen = APP_SCREEN_SESSION;
}

static void updateLobby(AppController* controller, u64 buttons)
{
    s32 count = getClampedNetworkCount(controller);

    if (count == 0) {
        controller->lobby_selection = 0;
        controller->lobby_scroll = 0;
        return;
    }

    appControllerMoveSelection(
        &controller->lobby_selection, &controller->lobby_scroll,
        buttons, count, APP_LOBBY_VISIBLE_CARDS
    );

    if (buttons & HidNpadButton_A)
        beginJoin(controller);
}

static void updateLogs(AppController* controller, u64 buttons)
{
    s32 max_scroll =
        (s32)ldnModelGetLogCount(controller->model) - APP_LOG_VISIBLE_LINES;

    if (max_scroll < 0)
        max_scroll = 0;

    if ((buttons & HidNpadButton_Up) && controller->log_scroll < max_scroll)
        controller->log_scroll++;

    if ((buttons & HidNpadButton_Down) && controller->log_scroll > 0)
        controller->log_scroll--;
}

static void runPendingJoin(AppController* controller)
{
    s32 index = controller->pending_join_index;

    controller->pending_join_index = APP_NO_PENDING_JOIN;
    ldnModelConnect(controller->model, index);
}

static void leaveSession(AppController* controller)
{
    ldnModelLeaveSession(controller->model);

    controller->screen = APP_SCREEN_BROWSE;
    controller->chat_panel = APP_CHAT_PANEL_NONE;
    resetScrollPositions(controller);
}

#define APP_HELLO_CHAT_TEXT "Hello!!"

static void sendHelloChat(AppController* controller)
{
    bool sent = ldnModelSendChat(controller->model, APP_HELLO_CHAT_TEXT);

    ldnModelLog(
        controller->model,
        sent ? "Chat sent: %s" : "ERROR: Chat send failed (not joined?): %s",
        APP_HELLO_CHAT_TEXT
    );
}

static void updateSession(AppController* controller, u64 buttons)
{
    if (controller->pending_join_index != APP_NO_PENDING_JOIN) {
        runPendingJoin(controller);
        return;
    }

    if (controller->chat_panel != APP_CHAT_PANEL_NONE) {
        appControllerChatUpdate(controller, buttons);
        return;
    }

    if (buttons & HidNpadButton_B) {
        leaveSession(controller);
        return;
    }

    if (buttons & HidNpadButton_X)
        sendHelloChat(controller);

    if (buttons & HidNpadButton_Y)
        appControllerChatOpenBrowser(controller);
}

bool appControllerUpdate(
    AppController* controller,
    PadState* pad
)
{
    u64 buttons = appControllerApplyDpadRepeat(
        controller, pad, padGetButtonsDown(pad)
    );

    /*
     * + must not exit while in a lobby session: leaving should go through B
     * so the Civ VII session and LDN connection are closed first.
     */
    if ((buttons & HidNpadButton_Plus) && controller->screen != APP_SCREEN_SESSION)
        return false;

    if (controller->screen == APP_SCREEN_SESSION) {
        updateSession(controller, buttons);
        return true;
    }

    updateTab(controller, buttons);
    refreshNetworks(controller, buttons);

    if (controller->tab == APP_TAB_LOBBY)
        updateLobby(controller, buttons);
    else
        updateLogs(controller, buttons);

    return true;
}
