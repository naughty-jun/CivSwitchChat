#include "app_controller_chat.h"
#include "app_controller_input.h"

#include <stdio.h>
#include <string.h>

#define CHAT_FILE_NOTICE_DURATION_MS 3000

static void showChatNotice(AppController* controller, const char* text)
{
    snprintf(controller->chat_file_notice, sizeof(controller->chat_file_notice), "%s", text);
    controller->chat_file_notice_until_ms = ldnModelNowMs() + CHAT_FILE_NOTICE_DURATION_MS;
}

/* ---- Browse ---- */

void appControllerChatOpenBrowser(AppController* controller)
{
    if (!chatFileListTxtFiles(controller->nro_directory, &controller->chat_file_list)) {
        ldnModelLog(controller->model, "Chat file list failed: cannot open %s",
                    controller->nro_directory);
        return;
    }

    if (controller->chat_file_list.truncated) {
        ldnModelLog(
            controller->model,
            "Chat file list: more than %d .txt files found, showing the first %d",
            CHAT_FILE_LIST_MAX, CHAT_FILE_LIST_MAX
        );
    }

    if (controller->chat_file_list.count == 0) {
        ldnModelLog(controller->model, "No .txt files found in %s", controller->nro_directory);
        return;
    }

    controller->chat_file_selection = 0;
    controller->chat_file_scroll = 0;
    controller->chat_panel = APP_CHAT_PANEL_BROWSE;
}

static void closeChatPanel(AppController* controller)
{
    controller->chat_panel = APP_CHAT_PANEL_NONE;
}

/* D-Pad Left/Right adjust the per-line send delay, clamped to the allowed range. */
static void adjustSendInterval(AppController* controller, u64 buttons)
{
    if (buttons & HidNpadButton_Left)
        controller->chat_send_interval_ms -= APP_CHAT_SEND_INTERVAL_STEP_MS;

    if (buttons & HidNpadButton_Right)
        controller->chat_send_interval_ms += APP_CHAT_SEND_INTERVAL_STEP_MS;

    if (controller->chat_send_interval_ms < APP_CHAT_SEND_INTERVAL_MIN_MS)
        controller->chat_send_interval_ms = APP_CHAT_SEND_INTERVAL_MIN_MS;

    if (controller->chat_send_interval_ms > APP_CHAT_SEND_INTERVAL_MAX_MS)
        controller->chat_send_interval_ms = APP_CHAT_SEND_INTERVAL_MAX_MS;
}

/* Starts sending the loaded file from its first line. */
static void beginSending(AppController* controller)
{
    controller->chat_send_index = 0;
    controller->chat_next_send_ms = ldnModelNowMs();
    controller->chat_panel = APP_CHAT_PANEL_SENDING;
}

static bool loadedFileNeedsWarning(const ChatFileContent* content)
{
    return chatFileNeedsSizeWarning(content->file_size) ||
           chatFileNeedsLineCountWarning(content->line_count) ||
           content->line_count_exceeds_limit;
}

/* Starts sending immediately, or opens the warning panel for a large file. */
static void startSendingOrWarn(AppController* controller)
{
    if (loadedFileNeedsWarning(&controller->chat_file_content)) {
        controller->chat_panel = APP_CHAT_PANEL_WARNING;
        return;
    }

    beginSending(controller);
}

/* Logs and shows a notice for a file that could not be loaded for sending. */
static void reportFileReadFailure(AppController* controller, const ChatFileEntry* entry)
{
    if (controller->chat_file_content.size_rejected) {
        ldnModelLog(
            controller->model,
            "%s is too large to open (%zu bytes, max %d)",
            entry->name,
            controller->chat_file_content.file_size,
            CHAT_FILE_SIZE_MAX_BYTES
        );
        showChatNotice(controller, "This file is too large to open.");
        return;
    }

    ldnModelLog(controller->model, "Failed to read %s", entry->name);
    showChatNotice(controller, "Could not read this file.");
}

static void loadSelectedFileAndStart(AppController* controller)
{
    const ChatFileEntry* entry =
        &controller->chat_file_list.entries[controller->chat_file_selection];

    if (!chatFileRead(controller->nro_directory, entry->name, &controller->chat_file_content)) {
        reportFileReadFailure(controller, entry);
        return;
    }

    if (controller->chat_file_content.line_count == 0) {
        ldnModelLog(controller->model, "%s has no text to send", entry->name);
        showChatNotice(controller, "This file has no text to send.");
        return;
    }

    snprintf(controller->chat_file_name, sizeof(controller->chat_file_name), "%s", entry->name);
    ldnModelLog(
        controller->model,
        "Loaded %s: %d line(s), %zu byte(s)",
        entry->name,
        (int)controller->chat_file_content.line_count,
        controller->chat_file_content.file_size
    );

    startSendingOrWarn(controller);
}

static void updateBrowse(AppController* controller, u64 buttons)
{
    if (buttons & HidNpadButton_B) {
        closeChatPanel(controller);
        return;
    }

    adjustSendInterval(controller, buttons);

    if (controller->chat_file_list.count == 0)
        return;

    appControllerMoveSelection(
        &controller->chat_file_selection, &controller->chat_file_scroll,
        buttons, controller->chat_file_list.count, APP_CHAT_FILE_VISIBLE_ROWS
    );

    if (buttons & HidNpadButton_A)
        loadSelectedFileAndStart(controller);
}

/* ---- Warning ---- */

static void updateWarning(AppController* controller, u64 buttons)
{
    if (buttons & HidNpadButton_B) {
        closeChatPanel(controller);
        return;
    }

    if (buttons & HidNpadButton_A)
        beginSending(controller);
}

/* ---- Sending ---- */

/* Logs and shows a notice for a line that is too long to send; does not send it. */
static void rejectLineTooLong(AppController* controller, s32 index, s32 total, const char* line)
{
    ldnModelLog(
        controller->model,
        "ERROR: Chat line too long, not sent (%d/%d, %d chars, max %d): %s",
        (int)(index + 1),
        (int)total,
        (int)strlen(line),
        CHAT_LINE_SEND_MAX_LENGTH,
        line
    );
    showChatNotice(controller, "Line too long, skipped (max 255 characters).");
}

static void sendNextLineIfDue(AppController* controller)
{
    const ChatFileContent* content = &controller->chat_file_content;

    if (controller->chat_send_index >= content->line_count)
        return;

    u64 now = ldnModelNowMs();
    if (now < controller->chat_next_send_ms)
        return;

    s32 index = controller->chat_send_index;
    const char* line = content->lines[index];

    if (chatFileLineTooLongToSend(line)) {
        rejectLineTooLong(controller, index, content->line_count, line);
    } else {
        bool sent = ldnModelSendChat(controller->model, line);

        ldnModelLog(
            controller->model,
            sent ? "Chat sent (%d/%d): %s" : "ERROR: Chat send failed (%d/%d): %s",
            (int)(index + 1),
            (int)content->line_count,
            line
        );

        if (!sent)
            showChatNotice(controller, "Chat send failed for one line.");
    }

    controller->chat_send_index++;
    controller->chat_next_send_ms = now + (u64)controller->chat_send_interval_ms;
}

static void updateSending(AppController* controller, u64 buttons)
{
    if (buttons & HidNpadButton_B) {
        closeChatPanel(controller);
        return;
    }

    sendNextLineIfDue(controller);

    if (controller->chat_send_index >= controller->chat_file_content.line_count)
        closeChatPanel(controller);
}

/* ---- Entry point ---- */

void appControllerChatUpdate(AppController* controller, u64 buttons)
{
    switch (controller->chat_panel) {
        case APP_CHAT_PANEL_BROWSE:
            updateBrowse(controller, buttons);
            return;
        case APP_CHAT_PANEL_WARNING:
            updateWarning(controller, buttons);
            return;
        case APP_CHAT_PANEL_SENDING:
            updateSending(controller, buttons);
            return;
        case APP_CHAT_PANEL_NONE:
            return;
    }
}
