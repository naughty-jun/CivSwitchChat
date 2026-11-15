#pragma once

/* Chat file panel (browse / warning / sending) shown on the session screen. */

#include "app_controller.h"

/* Opens the file browser; does nothing if there are no .txt files to list. */
void appControllerChatOpenBrowser(AppController* controller);

/* Handles input while chat_panel != APP_CHAT_PANEL_NONE. */
void appControllerChatUpdate(AppController* controller, u64 buttons);
