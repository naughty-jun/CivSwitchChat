#pragma once

#include <switch.h>

#include "../model/chat_file.h"
#include "../model/ldn_model.h"

#define APP_LOBBY_VISIBLE_CARDS 3
#define APP_LOG_VISIBLE_LINES 18
#define APP_NO_PENDING_JOIN -1
#define APP_CHAT_FILE_VISIBLE_ROWS 10

#define APP_CHAT_SEND_INTERVAL_DEFAULT_MS 300
#define APP_CHAT_SEND_INTERVAL_MIN_MS     50
#define APP_CHAT_SEND_INTERVAL_MAX_MS     1000
#define APP_CHAT_SEND_INTERVAL_STEP_MS    50

/* D-Pad hold-to-repeat timing, used for scrolling/adjusting lists and values. */
#define APP_DPAD_REPEAT_DELAY_MS    300 /* Time held before repeating starts. */
#define APP_DPAD_REPEAT_INTERVAL_MS 100 /* Time between repeats once started. */

typedef enum {
    APP_TAB_LOBBY = 0,
    APP_TAB_LOG = 1
} AppTab;

typedef enum {
    APP_SCREEN_BROWSE = 0,   /* Lobby list and logs */
    APP_SCREEN_SESSION = 1   /* Joining / joined lobby */
} AppScreen;

/* Sub-screen shown on top of the session screen while sending a chat file. */
typedef enum {
    APP_CHAT_PANEL_NONE = 0,    /* Session screen as usual */
    APP_CHAT_PANEL_BROWSE,      /* Picking a .txt file */
    APP_CHAT_PANEL_WARNING,     /* File is large; confirm before sending */
    APP_CHAT_PANEL_SENDING      /* Sending lines, one every chat_send_interval_ms */
} AppChatPanel;

typedef struct {
    AppScreen screen;
    AppTab tab;

    /* Network chosen with A; the connection starts on the next update. */
    s32 pending_join_index;

    s32 lobby_scroll;
    s32 lobby_selection;
    s32 log_scroll;

    /* ---- Chat file sending (session screen) ---- */
    AppChatPanel chat_panel;
    ChatFileList chat_file_list;
    s32 chat_file_selection;
    s32 chat_file_scroll;

    /*
     * Transient message shown over the chat file panels (browse or sending),
     * e.g. "file too large" or "line too long, not sent".
     */
    char chat_file_notice[128];
    u64 chat_file_notice_until_ms;

    ChatFileContent chat_file_content;
    char chat_file_name[CHAT_FILE_NAME_MAX];

    s32 chat_send_index;          /* Next line to send, or == line_count when done. */
    u64 chat_next_send_ms;
    s32 chat_send_interval_ms;    /* User-adjustable delay between lines. */

    /* Directory the .nro runs from; where chat .txt files are looked up. */
    char nro_directory[CHAT_FILE_NAME_MAX];

    /* D-Pad hold-to-repeat state, shared by every screen's scroll/adjust input. */
    u64 dpad_held_button;     /* The single D-Pad button currently tracked, or 0. */
    u64 dpad_next_repeat_ms;

    LdnModel* model;
} AppController;

void appControllerInitialize(
    AppController* controller,
    LdnModel* model,
    const char* nro_directory
);

bool appControllerUpdate(
    AppController* controller,
    PadState* pad
);
