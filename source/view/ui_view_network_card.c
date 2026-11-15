#include "ui_view_parts.h"
#include "ui_theme.h"

#include <stdio.h>
#include <string.h>

#define SSID_MAX_LENGTH  32

/* Card frame */
#define CARD_X               40
#define CARD_WIDTH           1200
#define CARD_HEIGHT          145
#define CARD_RADIUS          14
#define CARD_BORDER_SELECTED 3
#define CARD_BORDER_NORMAL   1

#define ACCENT_BAR_Y_OFFSET  16
#define ACCENT_BAR_WIDTH     4
#define ACCENT_BAR_HEIGHT    113
#define ACCENT_BAR_RADIUS    2

/* Left column */
#define NUMBER_X             66
#define NUMBER_Y_OFFSET      15
#define NUMBER_FONT_SIZE     15
#define NUMBER_MAX_WIDTH     50

#define LEFT_COLUMN_X        112
#define MAC_Y_OFFSET         12
#define MAC_FONT_SIZE        25
#define SSID_Y_OFFSET        49
#define SSID_FONT_SIZE       17
#define METADATA_Y_OFFSET    91
#define METADATA_FONT_SIZE   14
#define LEFT_TEXT_MAX_WIDTH  650
#define METADATA_MAX_WIDTH   750

/* Right column */
#define RIGHT_COLUMN_X            900
#define RIGHT_TEXT_MAX_WIDTH      280
#define JOIN_HINT_Y_OFFSET        15
#define JOIN_HINT_FONT_SIZE       14
#define PLAYERS_Y_OFFSET          54
#define PLAYERS_FONT_SIZE         17
#define CIV7_LOBBY_HINT_Y_OFFSET  84
#define CIV7_LOBBY_HINT_FONT_SIZE 14

static void copySsid(char* output, const LdnNetworkInfo* info)
{
    u8 length = info->common.ssid.len;

    if (length > SSID_MAX_LENGTH)
        length = SSID_MAX_LENGTH;

    memcpy(output, info->common.ssid.str, length);
    output[length] = '\0';
}

static void drawFrame(UiDrawContext* draw, s32 y, bool selected)
{
    uiDrawRoundedBorder(
        draw,
        CARD_X, y,
        CARD_WIDTH, CARD_HEIGHT,
        CARD_RADIUS,
        selected ? CARD_BORDER_SELECTED : CARD_BORDER_NORMAL,
        selected ? UI_COLOR_SELECTION : UI_COLOR_PANEL_BORDER,
        selected ? UI_COLOR_SELECTION_BG : UI_COLOR_PANEL
    );

    uiDrawRoundedRect(
        draw,
        CARD_X, y + ACCENT_BAR_Y_OFFSET,
        ACCENT_BAR_WIDTH, ACCENT_BAR_HEIGHT,
        ACCENT_BAR_RADIUS,
        selected ? UI_COLOR_SELECTION : UI_COLOR_ACCENT
    );
}

static void drawIdentity(
    UiDrawContext* draw,
    const LdnNetworkInfo* info,
    s32 index,
    s32 y,
    bool selected
)
{
    char number[16];
    char mac[LDN_MAC_TEXT_SIZE];
    char ssid[SSID_MAX_LENGTH + 1];
    char metadata[160];

    snprintf(number, sizeof(number), "%02d", index + 1);
    ldnModelFormatMacAddress(mac, sizeof(mac), info->common.bssid.addr);
    copySsid(ssid, info);
    snprintf(
        metadata,
        sizeof(metadata),
        "CH: %d  TYPE: %u  SECURITY: %u  COMM_ID: %lld",
        info->common.channel,
        info->common.network_type,
        info->security_mode,
        (long long)info->network_id.intent_id.local_communication_id
    );

    uiDrawText(
        draw, NUMBER_X, y + NUMBER_Y_OFFSET,
        number,
        NUMBER_FONT_SIZE,
        selected ? UI_COLOR_SELECTION : UI_COLOR_TEXT_MUTED,
        NUMBER_MAX_WIDTH
    );

    uiDrawText(
        draw, LEFT_COLUMN_X, y + MAC_Y_OFFSET,
        mac,
        MAC_FONT_SIZE,
        UI_COLOR_TEXT,
        LEFT_TEXT_MAX_WIDTH
    );

    uiDrawText(
        draw, LEFT_COLUMN_X, y + SSID_Y_OFFSET,
        ssid[0] ? ssid : "(Hidden SSID)",
        SSID_FONT_SIZE,
        UI_COLOR_ACCENT,
        LEFT_TEXT_MAX_WIDTH
    );

    uiDrawText(
        draw, LEFT_COLUMN_X, y + METADATA_Y_OFFSET,
        metadata,
        METADATA_FONT_SIZE,
        UI_COLOR_TEXT_SECONDARY,
        METADATA_MAX_WIDTH
    );
}

static void drawDetails(
    UiDrawContext* draw,
    const LdnNetworkInfo* info,
    s32 y,
    bool selected
)
{
    if (selected) {
        uiDrawText(
            draw, RIGHT_COLUMN_X, y + JOIN_HINT_Y_OFFSET,
            "Press [A] to Join",
            JOIN_HINT_FONT_SIZE,
            UI_COLOR_SELECTION,
            RIGHT_TEXT_MAX_WIDTH
        );
    }
    
    char players_text[96];
     snprintf(
        players_text,
        sizeof(players_text),
        "PLAYERS  %u / %u",
        info->node_count,
        info->node_count_max
    );
    uiDrawText(
        draw, RIGHT_COLUMN_X, y + PLAYERS_Y_OFFSET,
        players_text,
        PLAYERS_FONT_SIZE,
        UI_COLOR_TEXT,
        RIGHT_TEXT_MAX_WIDTH
    );

    const char *civ7_lobby_hint_text;
    u32 civ7_lobby_hint_color;

    if (info->network_id.intent_id.local_communication_id == CIV7_LDN_LOCAL_COMM_ID) {
        civ7_lobby_hint_text = "Likely a Civ VII lobby";
        civ7_lobby_hint_color = UI_COLOR_ACCENT;
    } else {
        civ7_lobby_hint_text = "Unlikely a Civ VII lobby";
        civ7_lobby_hint_color = UI_COLOR_TEXT_SECONDARY;
    }

    uiDrawText(
        draw, RIGHT_COLUMN_X, y + CIV7_LOBBY_HINT_Y_OFFSET,
        civ7_lobby_hint_text,
        CIV7_LOBBY_HINT_FONT_SIZE,
        civ7_lobby_hint_color,
        LEFT_TEXT_MAX_WIDTH
    );
    
}

void uiViewDrawNetworkCard(
    UiDrawContext* draw,
    const LdnNetworkInfo* info,
    s32 index,
    s32 y,
    bool selected
)
{
    drawFrame(draw, y, selected);
    drawIdentity(draw, info, index, y, selected);
    drawDetails(draw, info, y, selected);
}
