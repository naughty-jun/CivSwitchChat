#include "ui_view_parts.h"
#include "ui_layout.h"
#include "ui_theme.h"

#include <stdio.h>

/* Status column (left) */
#define SESSION_HEADING_Y         355
#define STEP_LIST_Y               400
#define STEP_STRIDE               40

#define STEP_DOT_X                48
#define STEP_DOT_Y_OFFSET         6
#define STEP_DOT_SIZE             12
#define STEP_DOT_RADIUS           6

#define STEP_LABEL_X              70
#define STEP_LABEL_FONT_SIZE      18
#define STEP_LABEL_MAX_WIDTH      220

#define STEP_STATUS_X             300
#define STEP_STATUS_Y_OFFSET      3
#define STEP_STATUS_FONT_SIZE     14
#define STEP_STATUS_MAX_WIDTH     260

/* Recent activity column (right) */
#define ACTIVITY_X                640
#define ACTIVITY_LIST_Y           400
#define ACTIVITY_LINE_COUNT       8
#define ACTIVITY_LINE_STRIDE      25
#define ACTIVITY_TEXT_MAX_WIDTH   580

#define SLOT_TEXT_SIZE            32

typedef enum {
    STEP_WAITING,
    STEP_ACTIVE,
    STEP_DONE,
    STEP_FAILED
} StepState;

/* ---- Step states derived from the session phase ---- */

static StepState getLdnStepState(LdnSessionPhase phase)
{
    switch (phase) {
        case LDN_SESSION_IDLE:
            return STEP_WAITING;
        case LDN_SESSION_CONNECTING_LDN:
            return STEP_ACTIVE;
        case LDN_SESSION_LDN_FAILED:
        case LDN_SESSION_LDN_DISCONNECTED:
            return STEP_FAILED;
        default:
            return STEP_DONE;
    }
}

static StepState getHandshakeStepState(LdnSessionPhase phase)
{
    switch (phase) {
        case LDN_SESSION_HANDSHAKING:
            return STEP_ACTIVE;
        case LDN_SESSION_JOINED:
            return STEP_DONE;
        case LDN_SESSION_UDP_FAILED:
        case LDN_SESSION_REFUSED:
        case LDN_SESSION_TIMED_OUT:
        case LDN_SESSION_LDN_DISCONNECTED:
            return STEP_FAILED;
        default:
            return STEP_WAITING;
    }
}

static StepState getJoinedStepState(LdnSessionPhase phase)
{
    return phase == LDN_SESSION_JOINED ? STEP_DONE : STEP_WAITING;
}

/* ---- Status texts ---- */

static const char* getLdnStatusText(LdnSessionPhase phase, StepState state)
{
    if (phase == LDN_SESSION_LDN_DISCONNECTED)
        return "DISCONNECTED";

    switch (state) {
        case STEP_ACTIVE:
            return "CONNECTING...";
        case STEP_DONE:
            return "CONNECTED";
        case STEP_FAILED:
            return "FAILED";
        default:
            return "WAITING";
    }
}

static const char* getHandshakeStatusText(LdnSessionPhase phase)
{
    switch (phase) {
        case LDN_SESSION_HANDSHAKING:
            return "WAITING FOR HOST...";
        case LDN_SESSION_JOINED:
            return "ACCEPTED";
        case LDN_SESSION_UDP_FAILED:
            return "UDP START FAILED";
        case LDN_SESSION_REFUSED:
            return "REFUSED";
        case LDN_SESSION_TIMED_OUT:
            return "TIMED OUT";
        case LDN_SESSION_LDN_DISCONNECTED:
            return "LOBBY CLOSED";
        default:
            return "WAITING";
    }
}

/* ---- Drawing ---- */

static u32 getStepColor(StepState state)
{
    switch (state) {
        case STEP_ACTIVE:
            return UI_COLOR_ACCENT;
        case STEP_DONE:
            return UI_COLOR_SUCCESS;
        case STEP_FAILED:
            return UI_COLOR_ERROR;
        default:
            return UI_COLOR_TEXT_MUTED;
    }
}

static void drawStep(
    UiDrawContext* draw,
    s32 y,
    const char* label,
    StepState state,
    const char* status_text
)
{
    u32 color = getStepColor(state);

    uiDrawRoundedRect(
        draw,
        STEP_DOT_X, y + STEP_DOT_Y_OFFSET,
        STEP_DOT_SIZE, STEP_DOT_SIZE,
        STEP_DOT_RADIUS,
        color
    );

    uiDrawText(
        draw, STEP_LABEL_X, y,
        label,
        STEP_LABEL_FONT_SIZE,
        state == STEP_WAITING ? UI_COLOR_TEXT_MUTED : UI_COLOR_TEXT,
        STEP_LABEL_MAX_WIDTH
    );

    uiDrawText(
        draw, STEP_STATUS_X, y + STEP_STATUS_Y_OFFSET,
        status_text,
        STEP_STATUS_FONT_SIZE,
        color,
        STEP_STATUS_MAX_WIDTH
    );
}

static void drawStatusSteps(UiDrawContext* draw, const LdnModel* model)
{
    LdnSessionPhase phase = model->session_phase;
    char slot_text[SLOT_TEXT_SIZE] = "WAITING";

    StepState ldn_state = getLdnStepState(phase);
    StepState handshake_state = getHandshakeStepState(phase);
    StepState joined_state = getJoinedStepState(phase);

    if (joined_state == STEP_DONE)
        snprintf(slot_text, sizeof(slot_text), "SLOT %u", (unsigned int)model->civ7.slot);

    uiViewDrawSectionHeading(
        draw, UI_SECTION_TITLE_X, SESSION_HEADING_Y, "Session Status",
        NULL, 0, 0
    );

    drawStep(draw, STEP_LIST_Y, "LDN connection",
             ldn_state, getLdnStatusText(phase, ldn_state));

    drawStep(draw, STEP_LIST_Y + STEP_STRIDE, "Civ VII handshake",
             handshake_state, getHandshakeStatusText(phase));

    drawStep(draw, STEP_LIST_Y + STEP_STRIDE * 2, "Lobby joined",
             joined_state, slot_text);
}

static void drawRecentActivity(UiDrawContext* draw, const LdnModel* model)
{
    u32 count = ldnModelGetLogCount(model);
    u32 start = count > ACTIVITY_LINE_COUNT ? count - ACTIVITY_LINE_COUNT : 0;
    s32 y = ACTIVITY_LIST_Y;

    uiViewDrawSectionHeading(
        draw, ACTIVITY_X, SESSION_HEADING_Y, "Recent Activity",
        NULL, 0, 0
    );

    for (u32 i = start; i < count; i++) {
        uiViewDrawLogLine(
            draw,
            ldnModelGetLogLine(model, i),
            ACTIVITY_X, y,
            ACTIVITY_TEXT_MAX_WIDTH
        );

        y += ACTIVITY_LINE_STRIDE;
    }
}

void uiViewDrawSession(
    UiDrawContext* draw,
    const AppController* controller,
    const LdnModel* model
)
{
    (void)controller;

    if (model->session_phase != LDN_SESSION_IDLE) {
        uiViewDrawNetworkCard(
            draw,
            &model->session_network,
            model->session_network_index,
            UI_CARD_LIST_Y,
            false
        );
    }

    drawStatusSteps(draw, model);
    drawRecentActivity(draw, model);
}
