#include "ldn_model_internal.h"

#define SESSION_NODE_POLL_INTERVAL_MS 2000

static void logStartFailure(LdnModel* model, Civ7StartResult start_result, int socket_errno)
{
    ldnModelLog(
        model,
        "Civ VII UDP session start failed: stage=%d errno=%d",
        start_result,
        socket_errno
    );
}

void ldnModelStartCiv7Session(LdnModel* model, const LdnNetworkInfo* network)
{
    LdnIpv4Address local_address = {0};
    LdnSubnetMask local_mask = {0};
    Result rc = ldnGetIpv4Address(&local_address, &local_mask);

    if (R_FAILED(rc)) {
        model->session_phase = LDN_SESSION_UDP_FAILED;
        ldnModelLog(
            model,
            "Civ VII UDP skipped: GetIpv4Address failed 0x%08X",
            (unsigned int)rc
        );
        return;
    }

    /* Civ VII's host is node 0 in the scanned LDN network record. */
    char local_ip[LDN_IPV4_TEXT_SIZE];
    char host_ip[LDN_IPV4_TEXT_SIZE];

    ldnModelFormatIpv4(local_ip, sizeof(local_ip), local_address);
    ldnModelFormatIpv4(host_ip, sizeof(host_ip), network->nodes[0].ip_addr);
    ldnModelLog(model, "Starting Civ VII UDP: %s -> %s:%u",
                local_ip, host_ip, CIV7_UDP_PORT);

    int socket_errno = 0;
    Civ7StartResult start_result = civ7SessionStart(&model->civ7, local_ip, host_ip, &socket_errno);

    model->civ7_started = (start_result == CIV7_START_OK);
    model->civ7_join_logged = false;
    model->civ7_failure_logged = false;

    if (!model->civ7_started) {
        model->session_phase = LDN_SESSION_UDP_FAILED;
        logStartFailure(model, start_result, socket_errno);
        return;
    }

    model->session_phase = LDN_SESSION_HANDSHAKING;
    model->session_next_node_poll_ms = civ7NowMs();
    ldnModelLog(model, "Civ VII FireCONN sent");
}

void ldnModelCloseCiv7Session(LdnModel* model)
{
    if (model->civ7_started)
        civ7SessionClose(&model->civ7);

    model->civ7_started = false;
}

#define SESSION_NODES_CAPACITY \
    (sizeof(((LdnModel*)0)->session_network.nodes) / sizeof(((LdnModel*)0)->session_network.nodes[0]))

/* Refreshes session_network's node list and count from the live LDN state. */
static void refreshSessionNetworkNodes(LdnModel* model)
{
    LdnNetworkInfo info = {0};
    Result rc = ldnGetNetworkInfo(&info);

    if (R_FAILED(rc))
        return;

    model->session_network.node_count = info.node_count;
    model->session_network.node_count_max = info.node_count_max;

    size_t copy_count = info.node_count;
    if (copy_count > SESSION_NODES_CAPACITY)
        copy_count = SESSION_NODES_CAPACITY;

    for (size_t i = 0; i < copy_count; i++)
        model->session_network.nodes[i] = info.nodes[i];
}

/* Only an active handshake or an established lobby can still lose the LDN link. */
static bool isSessionActive(LdnSessionPhase phase)
{
    return phase == LDN_SESSION_JOINED || phase == LDN_SESSION_HANDSHAKING;
}

/*
 * The host can drop the wireless LDN link itself (closing the lobby, turning
 * off Wi-Fi, crashing, ...) without Civ VII ever sending a refusal message.
 * Checking ldnGetState alongside the node count catches that case too.
 * Returns true when it detected the link is gone (caller should stop here).
 */
static bool detectLdnLinkLost(LdnModel* model)
{
    if (!isSessionActive(model->session_phase))
        return false;

    LdnState state = LdnState_None;
    Result rc = ldnGetState(&state);

    if (R_SUCCEEDED(rc) && state == LdnState_StationConnected)
        return false;

    model->session_phase = LDN_SESSION_LDN_DISCONNECTED;
    model->civ7_failure_logged = true;

    if (R_FAILED(rc)) {
        ldnModelLog(model, "Lobby connection lost: GetState failed 0x%08X", (unsigned int)rc);
    } else {
        ldnModelLog(
            model,
            "Lobby connection lost: LDN state is now %s",
            ldnModelGetStateName(state)
        );
    }

    ldnModelCloseCiv7Session(model);
    return true;
}

/* Keeps session_network fresh and watches for the LDN link dropping, every few seconds. */
static void pollSessionStateIfDue(LdnModel* model)
{
    uint64_t now = civ7NowMs();

    if (now < model->session_next_node_poll_ms)
        return;

    model->session_next_node_poll_ms = now + SESSION_NODE_POLL_INTERVAL_MS;

    if (detectLdnLinkLost(model))
        return;

    refreshSessionNetworkNodes(model);
}

/* Advances the phase to JOINED and logs it, the first time civ7 reports joined. */
static void logJoinOnce(LdnModel* model)
{
    if (!model->civ7.joined || model->civ7_join_logged)
        return;

    model->civ7_join_logged = true;
    model->session_phase = LDN_SESSION_JOINED;
    ldnModelLog(model, "Civ VII lobby joined (slot=%u)",
                (unsigned int)model->civ7.slot);
    ldnModelLog(model, "Game registration sequence scheduled");
}

/* Advances the phase to REFUSED/TIMED_OUT and logs it, the first time civ7 fails. */
static void logFailureOnce(LdnModel* model)
{
    bool refused = model->civ7.refused;
    bool failed = refused || model->civ7.timed_out;

    if (!failed || model->civ7_failure_logged)
        return;

    model->civ7_failure_logged = true;
    model->session_phase = refused ? LDN_SESSION_REFUSED : LDN_SESSION_TIMED_OUT;
    ldnModelLog(model, "%s", refused
                ? "Civ VII lobby refused/closed the connection"
                : "Civ VII lobby handshake timed out");
}

/* Logs when the peer asks us to resend a chat line (likely means it was not received yet). */
static void logResendNoticeIfAny(LdnModel* model)
{
    char text[CIV7_CHAT_MAX_LENGTH + 1];
    uint64_t seq = 0;

    if (civ7SessionTakeResendNotice(&model->civ7, text, sizeof(text), &seq)) {
        ldnModelLog(
            model,
            "Peer requested a resend (seq=%llu), chat may not have arrived yet: %s",
            (unsigned long long)seq,
            text
        );
    }
}

void ldnModelUpdate(LdnModel* model)
{
    if (!model->civ7_started)
        return;

    civ7SessionUpdate(&model->civ7);
    logJoinOnce(model);
    logFailureOnce(model);
    pollSessionStateIfDue(model);
    logResendNoticeIfAny(model);
}

bool ldnModelSendChat(LdnModel* model, const char* text)
{
    return model->civ7_started && civ7SessionSendChat(&model->civ7, text);
}

uint64_t ldnModelNowMs(void)
{
    return civ7NowMs();
}
